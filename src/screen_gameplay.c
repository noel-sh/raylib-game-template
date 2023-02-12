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

#define RAYLIB_ASEPRITE_IMPLEMENTATION
#include "raylib-aseprite.h"


#include <stdio.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;
static struct ldtk_world* world = NULL;
static Texture worldTextures[16] = { 0 };
static Aseprite worldSprites[16] = { 0 };
static Vector2 cameraOffset = { 0 };
static float cameraZoom = 1.0f;
static int worldDepthToShow = 0;

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

static void DrawTile(ldtk_layer_instance* inst, ldtk_tile* tile, Vector2 offset)
{
	if (inst->tileset == NULL) return;

	Rectangle src = { (float)tile->src_x, (float)tile->src_y, (float)inst->grid_size, (float)inst->grid_size };
	Rectangle dst = { (float)tile->px_x + offset.x, (float)tile->px_y + offset.y, (float)inst->grid_size, (float)inst->grid_size };

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

	Texture tex = *(Texture*)inst->tileset->userdata;
	DrawTexturePro(tex, src, dst, (Vector2) { 0.0f, 0.0f }, 0.0f, WHITE);
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
				DrawTile(inst, tile, layerOffset);
			}

			for (int k = 0; k < inst->gridtile_count; ++k)
			{
				ldtk_tile* tile = &inst->gridtiles[k];
				DrawTile(inst, tile, layerOffset);
			}

		}
	}
}



//////////////////////////////////////////////////////////////////////////
// Input


typedef struct PlayerInputType
{
	bool bMoveLeft;
	bool bMoveRight;
	bool bJump;
} PlayerInputType;


PlayerInputType GetPlayerInput()
{
	PlayerInputType input;
	memset(&input, 0, sizeof(input));

	if (IsGamepadAvailable(0))
	{
		if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < -0.5f)
		{
			input.bMoveLeft = true;
		}
		if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) > 0.5f)
		{
			input.bMoveRight = true;
		}
		if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
		{
			input.bJump = true;
		}
	}
	{
		if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
		{
			input.bMoveLeft = true;
		}
		if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
		{
			input.bMoveRight = true;
		}
		if (IsKeyDown(KEY_SPACE))
		{
			input.bJump = true;
		}
	}

	return input;
}



//////////////////////////////////////////////////////////////////////////
// Player stuff

typedef enum EPlayerJumpState
{
	PJS_None,
	PJS_JumpAscending,
	PJS_JumpApex,
	PJS_Falling
} EPlayerJumpState;

typedef struct PlayerType
{
	// runtime vars
	Vector2 Location;
	Vector2 Velocity;
	int JumpCount;
	EPlayerJumpState JumpState;
	int JumpStateTime;
	int NotGroundedTime;
	bool bIsJumping;

	// was Jump pressed last frame?
	bool bJumpPrev;
} PlayerType;


typedef struct GameState
{
	PlayerType Player;
	PlayerInputType Input;
} GameState;



//////////////////////////////////////////////////////////////////////////

void InitGame(GameState* state)
{
	memset(state, 0, sizeof(state));
	state->Player.Location = (Vector2){ 24, -26 };
}


GameState StepGame(GameState state)
{
	// start by copying the previous gamestate
	GameState newState = state;

	// update player
	//UpdatePlayer(newState);

	// track some random stats...
	//if (gStat_MaxHeight > newState.Player.Location.y)
	//{
	//	gStat_MaxHeight = newState.Player.Location.y;
	//}

	return newState;
}



//////////////////////////////////////////////////////////////////////////
// Main loop


// some debug stuff
static int gCurrentFrame = 0;
#define kMaxGameStates (60 * 60 * 10)	// 60 FPS * 60s * 10 = 10mins capacity (currently ~1.4mb)
static GameState gGameStates[kMaxGameStates] = {0};

int gGameStateCount = 0;

typedef enum EStepMode
{
	StepMode_Play,
	StepMode_Paused,
	StepMode_Replay,
} EStepMode;

EStepMode gStepMode = StepMode_Play;

bool gDebugUI_Timeline = false;




// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
	// TODO: Initialize GAMEPLAY screen variables here!
	framesCounter = 0;
	finishScreen = 0;

	world = ldtk_load_world("resources/WorldMap_Free_layout.ldtk");

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

			if (strstr(texturePath, ".aseprite"))
			{
				worldSprites[i] = LoadAseprite(texturePath);
				worldTextures[i] = GetAsepriteTexture(worldSprites[i]);
				tileset->userdata = &worldTextures[i];
			}
			else
			{
				worldTextures[i] = LoadTexture(texturePath);
				tileset->userdata = &worldTextures[i];
			}
		}
	}

	// setup initial gamestate
	GameState gameState;
	InitGame(&gameState);
	gGameStates[0] = gameState;
	gGameStateCount = 1;
	gCurrentFrame = 0;
}


// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    // TODO: Update GAMEPLAY screen variables here!
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		cameraOffset.x += GetMouseDelta().x / cameraZoom;
		cameraOffset.y += GetMouseDelta().y / cameraZoom;
	}

	cameraZoom += GetMouseWheelMoveV().y * 0.1f;

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }

	if (gStepMode == StepMode_Play)
	{
		GameState gameState = gGameStates[gCurrentFrame];
		gameState.Input = GetPlayerInput();
		gameState = StepGame(gameState);

		// store this gamestate!
		if ((gCurrentFrame + 1) < kMaxGameStates)
		{
			gCurrentFrame++;
			gGameStates[gCurrentFrame] = gameState;
			gGameStateCount = gCurrentFrame + 1;
		}
	}

	if (gStepMode == StepMode_Replay)
	{
		if (gCurrentFrame < gGameStateCount)
		{
			gCurrentFrame++;
		}
		else
		{
			// stop when we reach the end
			gStepMode = StepMode_Paused;
		}
	}
}

/*
// TODO - Requires RayGui
void DrawDebugUI()
{
	if (!gDebugUI_Timeline) return;

	char frameTxt[1024];
	float y = GetScreenHeight() - 70;
	float x = GetScreenWidth() / 2 - 512;
	float iw = 32;

	// jump to start
	if (GuiButton({ x, y, iw, iw }, "#129#"))
	{
		gStepMode = StepMode_Paused;
		gCurrentFrame = 0;
		gameState = gGameStates[gCurrentFrame];
	}
	// step backwards
	if (GuiButton({ x += iw, y, iw, iw }, "#114#"))
	{
		gStepMode = EStepMode::Paused;
		if (gCurrentFrame > 0)
		{
			gCurrentFrame--;
			gameState = gGameStates[gCurrentFrame];
		}
	}
	// step forwards
	if (GuiButton({ x += iw, y, iw, iw }, "#115#"))
	{
		gStepMode = EStepMode::Paused;
		if (gCurrentFrame < (gGameStateCount - 1))
		{
			gCurrentFrame++;
			gameState = gGameStates[gCurrentFrame];
		}
		else
		{
			// create a new gamestate!
			gameState = StepGame(gameState);

			if (gCurrentFrame < gMaxGameStates)
			{
				gCurrentFrame++;
				gGameStates[gCurrentFrame] = gameState;
				gGameStateCount = gCurrentFrame + 1;
			}
		}
	}
	// pause
	if (GuiButton({ x += iw, y, iw, iw }, "#132#"))
	{
		gStepMode = EStepMode::Paused;
	}
	// replay from current location
	if (GuiButton({ x += iw, y, iw, iw }, "#131#"))
	{
		gStepMode = EStepMode::Replay;
	}
	// jump to end
	if (GuiButton({ x += iw, y, iw, iw }, "#134#"))
	{
		gStepMode = EStepMode::Paused;
		gCurrentFrame = gGameStateCount - 1;
		gameState = gGameStates[gCurrentFrame];
	}
	// resume gameplay from here
	if (GuiButton({ x += iw, y, iw, iw }, "#150#"))
	{
		gStepMode = EStepMode::Play;
	}
	x += iw + 16;

	switch (gStepMode)
	{
	case EStepMode::Play:	strcpy(frameTxt, "PLAYING");	break;
	case EStepMode::Paused:	strcpy(frameTxt, "PAUSED");		break;
	case EStepMode::Replay:	strcpy(frameTxt, "REPLAY");		break;
	default:	frameTxt[0] = 0;
	}
	GuiDrawText(frameTxt, { x, y, 64, 32 }, TEXT_ALIGN_LEFT, RAYWHITE);
	x += 64;

	sprintf(frameTxt, "%d / %d", gCurrentFrame, gGameStateCount - 1);
	GuiDrawText(frameTxt, { x, y, 64, 32 }, TEXT_ALIGN_LEFT, RAYWHITE);
	x += 64;

	// show player inputs
	x += 16;
	gameState.Input.bJump = GuiCheckBox({ x, y, 16, 16 }, "Jump", gameState.Input.bJump);
	y += 16;
	gameState.Input.bMoveLeft = GuiCheckBox({ x, y, 16, 16 }, "Left", gameState.Input.bMoveLeft);
	x += 48;
	gameState.Input.bMoveRight = GuiCheckBox({ x, y, 16, 16 }, "Right", gameState.Input.bMoveRight);


	// timeline slider allows scrubbing thru saved states
	{
		if (gStepMode == EStepMode::Play)
		{
			// do not allow setting slider values if playing!
			GuiSetState(STATE_DISABLED);
		}

		sprintf(frameTxt, "%d", gGameStateCount - 1);
		float v = GuiSlider({ 128, kScreenHeight - 32, kScreenWidth - 256, 16 }, "0", frameTxt, (float)gCurrentFrame, 0.0f, (float)gGameStateCount);
		if ((int)v != gCurrentFrame && (int)v < gGameStateCount)
		{
			gCurrentFrame = (int)v;
			gameState = gGameStates[gCurrentFrame];
		}

		GuiSetState(STATE_NORMAL);
	}
}
*/



// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

	Vector2 target = {
		-cameraOffset.x,
		-cameraOffset.y
	};

	Camera2D camera = { 
		.offset = (Vector2) { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f },
		.target = target,
		.zoom = cameraZoom 
	};
	BeginMode2D(camera);
		DrawLevels();
	EndMode2D();

    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    //DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

	DrawFPS(GetScreenWidth() - 100, 10);
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!

	for (int i = 0; i < 16; ++i)
	{
		UnloadTexture(worldTextures[i]);
		UnloadAseprite(worldSprites[i]);
	}

    ldtk_destroy_world(world);
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}

