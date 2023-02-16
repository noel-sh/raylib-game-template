// A simple C API for working with ldtk files

// External types

struct ldtk_world;

typedef struct ldtk_level
{
	const char* identifier;
	int uid;
	int worldX;
	int worldY;
	int worldDepth;
	int pxWid;
	int pxHei;
	int layer_instances_count;
	struct ldtk_layer_instance* layer_instances;
} ldtk_level;

typedef struct ldtk_tileset
{
	const char* identifier;
	int uid;
	const char* relPath;
	int pxWid;
	int pxHei;
	int tileGridSize;
	int spacing;
	int padding;
	void* userdata;
} ldtk_tileset;

typedef struct ldtk_layer_instance
{
	const char* identifier;
	const char* type;
	int cWid;
	int cHei;
	int grid_size;
	int level_id;
	int layer_def_uid;
	int px_offset_x;
	int px_offset_y;

	int gridtile_count;
	struct ldtk_tile* gridtiles;

	int autotile_count;
	struct ldtk_tile* autotiles;

	// there are (cWid * cHei) cells if type is intgrid
	int* int_grid;

	struct ldtk_tileset* tileset;
} ldtk_layer_instance;

typedef struct ldtk_tile
{
	int px_x;
	int px_y;
	int src_x;
	int src_y;
	int f;
	int t;
} ldtk_tile;


#if defined(__cplusplus)
extern "C" {
#endif


struct ldtk_world* ldtk_load_world(const char* filename);
void ldtk_destroy_world(struct ldtk_world* world);

int ldtk_get_tileset_count(struct ldtk_world* world);
struct ldtk_tileset* ldtk_get_tileset(struct ldtk_world* world, int index);

int ldtk_get_level_count(struct ldtk_world* world);
struct ldtk_level* ldtk_get_level(struct ldtk_world* world, int index);



#if defined(__cplusplus)
}
#endif
