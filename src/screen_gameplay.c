/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "ldtk.h"
#include <stdio.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;
static struct ldtk_world* world = NULL;
static Texture worldTextures[16] = { 0 };
static Vector2 cameraOffset = { 0 };
static float cameraZoom = 1.0f;
static int worldDepthToShow = 0;

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    world = ldtk_load_world("resources/Typical_TopDown_example.ldtk");

	if (world)
	{
		// load textures for tilesets and store pointer in tileset userdata
		char texturePath[260];
		int count = ldtk_get_tileset_count(world);
		if (count > 16) count = 16;

		for (int i = 0; i < count; ++i)
		{
			struct ldtk_tileset* tileset = ldtk_get_tileset(world, i);
			sprintf(texturePath, "resources/%s", tileset->relPath);
            worldTextures[i] = LoadTexture(texturePath);
			tileset->userdata = &worldTextures[i];
		}

		// find the highest level depth
		worldDepthToShow = -999999;
		int levelCount = ldtk_get_level_count(world);
		for (int i = 0; i < levelCount; ++i)
		{
			int levelDepth = ldtk_get_level(world, i)->worldDepth;
			if (levelDepth > worldDepthToShow)
			{
				worldDepthToShow = levelDepth;
			}
		}
	}
}


void DrawLevels()
{
	// draw everything for now
	if (!world) return;

	int count = ldtk_get_level_count(world);
	for (int i = 0; i < count; ++i)
	{
		ldtk_level* level = ldtk_get_level(world, i);

		// render layers back to front
		for (int j = level->layer_instances_count-1; j >= 0 ; --j)
		{
			ldtk_layer_instance* inst = &level->layer_instances[j];
			Texture* tex = NULL;

			if (level->worldDepth != worldDepthToShow)
				continue;

			if (inst->tileset)
			{
				tex = (Texture*)inst->tileset->userdata;
			}

			Vector2 layerOffset = { (float)level->worldX + inst->px_offset_x, (float)level->worldY + inst->px_offset_y };

			for (int k = 0; k < inst->autotile_count; ++k)
			{
				ldtk_tile* tile = &inst->autotiles[k];
				if (tex)
				{
					Rectangle src = { (float)tile->src_x, (float)tile->src_y, (float)inst->grid_size, (float)inst->grid_size };
					Rectangle dst = { (float)tile->px_x + layerOffset.x, (float)tile->px_y + layerOffset.y, (float)inst->grid_size, (float)inst->grid_size };

					if (tile->f & 1)
					{
						// flip horizontally
						src.width *= -1;
					}
					if (tile->f & 2)
					{
						// flip vertically
						src.height *= -1;
					}

					DrawTexturePro(*tex, src, dst, (Vector2) { 0.0f, 0.0f }, 0.0f, WHITE);
				}
			}

			for (int k = 0; k < inst->gridtile_count; ++k)
			{
				ldtk_tile* tile = &inst->gridtiles[k];
				if (tex)
				{
					Rectangle src = { (float)tile->src_x, (float)tile->src_y, (float)inst->grid_size, (float)inst->grid_size };
					Rectangle dst = { (float)tile->px_x + layerOffset.x, (float)tile->px_y + layerOffset.y, (float)inst->grid_size, (float)inst->grid_size };

					if (tile->f & 1)
					{
						// flip horizontally
						src.width *= -1;
					}
					if (tile->f & 2)
					{
						// flip vertically
						src.height *= -1;
					}

					DrawTexturePro(*tex, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
					//DrawTextureRec(*tex, src, dst, WHITE);
				}
			}

		}
	}
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    // TODO: Update GAMEPLAY screen variables here!
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		cameraOffset.x += GetMouseDelta().x;
		cameraOffset.y += GetMouseDelta().y;
	}

	cameraZoom += GetMouseWheelMoveV().y * 0.1f;

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

	Camera2D camera = { 
		.offset = cameraOffset,
		.zoom = cameraZoom 
	};
	BeginMode2D(camera);
		DrawLevels();
	EndMode2D();

    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    //DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!

    ldtk_destroy_world(world);
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}