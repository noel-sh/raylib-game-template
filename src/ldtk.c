
#include "ldtk.h"
#include "external/parson.h"
#include <stdlib.h>
#include <string.h>


// Internal type holding all context information about a specific world
struct ldtk_world
{
	int level_count;
	struct ldtk_level* levels;

	int tileset_count;
	struct ldtk_tileset* tilesets;

	// keep this around so the strings remain valid
	JSON_Value* json_root;
};



// Internal functions

static void _ltdk_destroy_layer_instance(struct ldtk_layer_instance* inst)
{
	if (inst)
	{
		free(inst->gridtiles);
		free(inst->autotiles);
		free(inst->int_grid);
	}
}

static void _ltdk_destroy_level(struct ldtk_level* level)
{
	if (level)
	{
		for (int i = 0; i < level->layer_instances_count; ++i)
		{
			_ltdk_destroy_layer_instance(&level->layer_instances[i]);
		}
		free(level->layer_instances);
	}
}

static void _ltdk_destroy_world(struct ldtk_world* world)
{
	if (world)
	{
		if (world->levels)
		{
			for (int i = 0; i < world->level_count; ++i)
			{
				_ltdk_destroy_level(&world->levels[i]);
			}
			free(world->levels);
		}

		if (world->tilesets)
		{
			free(world->tilesets);
		}

		if (world->json_root)
		{
			json_value_free(world->json_root);
		}
	}
}

static int _ltdk_parse_tilesets(struct ldtk_world* world, JSON_Array* tilesets_arr)
{
	if (!world) return -1;
	int count = (int)json_array_get_count(tilesets_arr);

	world->tileset_count = count;
	world->tilesets = calloc(count, sizeof(struct ldtk_tileset));
	if (!world->tilesets) return -1;

	for (int i = 0; i < count; ++i)
	{
		JSON_Object* tileset_obj = json_array_get_object(tilesets_arr, i);
		struct ldtk_tileset* tileset = &world->tilesets[i];

		tileset->identifier = json_object_get_string(tileset_obj, "identifier");
		tileset->uid = (int)json_object_get_number(tileset_obj, "uid");
		tileset->relPath = json_object_get_string(tileset_obj, "relPath");
		tileset->pxWid = (int)json_object_get_number(tileset_obj, "pxWid");
		tileset->pxHei = (int)json_object_get_number(tileset_obj, "pxHei");
		tileset->tileGridSize = (int)json_object_get_number(tileset_obj, "tileGridSize");
		tileset->spacing = (int)json_object_get_number(tileset_obj, "spacing");
		tileset->padding = (int)json_object_get_number(tileset_obj, "padding");
	}

	return 0;
}

static int _ltdk_parse_tiles(struct ldtk_tile* tiles, JSON_Array* tiles_arr)
{
	if (!tiles) return -1;
	int count = (int)json_array_get_count(tiles_arr);
	for (int i = 0; i < count; ++i)
	{
		JSON_Object* tile_obj = json_array_get_object(tiles_arr, i);
		struct ldtk_tile* tile = &tiles[i];

		JSON_Array* px_arr = json_object_get_array(tile_obj, "px");
		tile->px_x = (int)json_array_get_number(px_arr, 0);
		tile->px_y = (int)json_array_get_number(px_arr, 1);

		JSON_Array* src_arr = json_object_get_array(tile_obj, "src");
		tile->src_x = (int)json_array_get_number(src_arr, 0);
		tile->src_y = (int)json_array_get_number(src_arr, 1);

		tile->f = (int)json_object_get_number(tile_obj, "f");
		tile->t = (int)json_object_get_number(tile_obj, "t");
	}
	return 0;
}

static int _ltdk_parse_layer_instances(struct ldtk_world* world, struct ldtk_level* level, JSON_Array* instances_arr)
{
	if (instances_arr)
	{
		level->layer_instances_count = (int)json_array_get_count(instances_arr);
		level->layer_instances = calloc(level->layer_instances_count, sizeof(struct ldtk_layer_instance));
		if (!level->layer_instances) return -1;

		for (int inst_i = 0; inst_i < level->layer_instances_count; ++inst_i)
		{
			JSON_Object* inst_obj = json_array_get_object(instances_arr, inst_i);
			struct ldtk_layer_instance* inst = &level->layer_instances[inst_i];

			inst->identifier = json_object_get_string(inst_obj, "__identifier");
			inst->type = json_object_get_string(inst_obj, "__type");
			inst->cWid = (int)json_object_get_number(inst_obj, "__cWid");
			inst->cHei = (int)json_object_get_number(inst_obj, "__cHei");
			inst->grid_size = (int)json_object_get_number(inst_obj, "__gridSize");
			inst->level_id = (int)json_object_get_number(inst_obj, "levelId");
			inst->layer_def_uid = (int)json_object_get_number(inst_obj, "layerDefUid");
			inst->px_offset_x = (int)json_object_get_number(inst_obj, "pxOffsetX");		// would __pxTotalOffsetY be more useful?
			inst->px_offset_y = (int)json_object_get_number(inst_obj, "pxOffsetY");

			// fixup tileset ptr
			if (json_object_has_value_of_type(inst_obj, "__tilesetDefUid", JSONNumber) == 1)
			{
				int tilesetUid = (int)json_object_get_number(inst_obj, "__tilesetDefUid");
				for (int i = 0; i < world->tileset_count; ++i)
				{
					if (world->tilesets[i].uid == tilesetUid)
					{
						inst->tileset = &world->tilesets[i];
						break;
					}
				}
			}

			// GridTiles
			JSON_Array* grid_tiles_arr = json_object_get_array(inst_obj, "gridTiles");
			if (grid_tiles_arr && json_array_get_count(grid_tiles_arr) > 0)
			{
				inst->gridtile_count = (int)json_array_get_count(grid_tiles_arr);
				inst->gridtiles = calloc(inst->gridtile_count, sizeof(struct ldtk_tile));
				if (_ltdk_parse_tiles(inst->gridtiles, grid_tiles_arr) < 0) return -1;
			}

			// AutoLayerTiles
			JSON_Array* autolayer_tiles_arr = json_object_get_array(inst_obj, "autoLayerTiles");
			if (autolayer_tiles_arr && json_array_get_count(autolayer_tiles_arr) > 0)
			{
				inst->autotile_count = (int)json_array_get_count(autolayer_tiles_arr);
				inst->autotiles = calloc(inst->autotile_count, sizeof(struct ldtk_tile));
				if (_ltdk_parse_tiles(inst->autotiles, autolayer_tiles_arr) < 0) return -1;
			}

			// parse IntGrid type
			if (strcmp(inst->type, "IntGrid") == 0)
			{
				int intgrid_cell_count = inst->cWid * inst->cHei;
				JSON_Array* intgrid_arr = json_object_get_array(inst_obj, "intGridCsv");
				if (!intgrid_arr || json_array_get_count(intgrid_arr) != intgrid_cell_count)
				{
					return -2;
				}

				inst->int_grid = calloc(intgrid_cell_count, sizeof(int));
				if (!inst->int_grid) return -1;

				for (int cell_i = 0; cell_i < intgrid_cell_count; ++cell_i)
				{
					inst->int_grid[cell_i] = (int)json_array_get_number(intgrid_arr, cell_i);
				}
			}

		}
	}
	return 0;
}

static int _ltdk_parse_levels(struct ldtk_world* world, JSON_Array* levels_array)
{
	world->level_count = (int)json_array_get_count(levels_array);
	world->levels = calloc(world->level_count, sizeof(struct ldtk_level));
	if (!world->levels) return -1;

	for (int level_i = 0; level_i < world->level_count; ++level_i)
	{
		JSON_Object* level_obj = json_array_get_object(levels_array, level_i);
		struct ldtk_level* level = &world->levels[level_i];

		level->identifier = json_object_get_string(level_obj, "identifier");
		level->uid = (int)json_object_get_number(level_obj, "uid");
		level->worldX = (int)json_object_get_number(level_obj, "worldX");
		level->worldY = (int)json_object_get_number(level_obj, "worldY");
		level->worldDepth = (int)json_object_get_number(level_obj, "worldDepth");
		level->pxWid = (int)json_object_get_number(level_obj, "pxWid");
		level->pxHei = (int)json_object_get_number(level_obj, "pxHei");

		JSON_Array* instances_arr = json_object_get_array(level_obj, "layerInstances");

		int err = _ltdk_parse_layer_instances(world, level, instances_arr);
		if (err < 0) return err;
	}
	return 0;
}

static struct ldtk_world* _ltdk_parse_world(JSON_Value* root)
{
	struct ldtk_world* world = NULL;
	if (root)
	{
		world = calloc(1, sizeof(struct ldtk_world));
		if (!world) goto load_world_err;

		world->json_root = root;

		JSON_Object* defs_obj = json_object_get_object(json_object(root), "defs");
		if (!defs_obj) goto load_world_err;

		// load tilesets data
		if (_ltdk_parse_tilesets(world, json_object_get_array(defs_obj, "tilesets")) < 0) goto load_world_err;

		JSON_Array* levels_array = json_object_get_array(json_object(root), "levels");
		if (!levels_array) goto load_world_err;

		// load levels
		if (_ltdk_parse_levels(world, levels_array) < 0) goto load_world_err;
	}

	return world;

load_world_err:
	_ltdk_destroy_world(world);
	return NULL;
}






// External functions

struct ldtk_world* ldtk_load_world(const char* filename)
{
	JSON_Value* json_root = json_parse_file(filename);
	if (json_root)
	{
		struct ldtk_world* world = _ltdk_parse_world(json_root);
		return world;
	}
	return NULL;
}


void ldtk_destroy_world(struct ldtk_world* world)
{
	_ltdk_destroy_world(world);
}


int ldtk_get_tileset_count(struct ldtk_world* world)
{
	if (world) return world->tileset_count;
	return 0;
}


struct ldtk_tileset* ldtk_get_tileset(struct ldtk_world* world, int index)
{
	if (world && world->tileset_count > index)
	{
		return &world->tilesets[index];
	}
	return NULL;
}


int ldtk_get_level_count(struct ldtk_world* world)
{
	if (world) return world->level_count;
	return 0;
}


struct ldtk_level* ldtk_get_level(struct ldtk_world* world, int index)
{
	if (world && world->level_count > index)
	{
		return &world->levels[index];
	}
	return NULL;
}

