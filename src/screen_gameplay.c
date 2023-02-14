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
#include "raymath.h"

#define RAYLIB_ASEPRITE_IMPLEMENTATION
#include "raylib-aseprite.h"

#define RAYGUI_IMPLEMENTATION
#include "external/raygui.h"


#include <stdio.h>
#include <float.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static struct ldtk_world* gWorld = NULL;
static Texture gWorldTextures[16] = { 0 };
static Aseprite gWorldSprites[16] = { 0 };



//////////////////////////////////////////////////////////////////////////
// Test area for collision functions!


typedef struct TraceResult {
	Vector2 start;
	Vector2 end;
	Vector2 hitPos;		// world position of the hit
	Vector2 hitNormal;	// world normal of the surface that was hit
	float dist;			// distance from Start where the hit occurred
	int gridValue;		// if we hit an int_grid cell, what was the value
	bool hasHit;		// did the trace hit anything?
} TraceResult;


static TraceResult WorldTrace(struct ldtk_world* world, Vector2 start, Vector2 end, bool bDebugDraw)
{
	Rectangle rayBounds = {
		.x = start.x < end.x ? start.x : end.x,
		.y = start.y < end.y ? start.y : end.y,
		.width = fabsf(start.x - end.x),
		.height = fabsf(start.y - end.y)
	};

	TraceResult result = {
		.start = start,
		.end = end,
		.hasHit = false,
		.dist = FLT_MAX
	};

	// iterate all level instances and check which ones contain the bounding box
	int count = ldtk_get_level_count(world);
	for (int i = 0; i < count; ++i)
	{
		ldtk_level* level = ldtk_get_level(world, i);
		for (int j = 0; j < level->layer_instances_count; ++j)
		{
			ldtk_layer_instance* inst = &level->layer_instances[j];
			if (inst->int_grid)
			{
				// assuming this int_grid is for collisions... idk how to know this!?
				// game specific I guess

				int grid_size = inst->grid_size;
				float gridF = (float)grid_size;

				int instWorldX = level->worldX + inst->px_offset_x;
				int instWorldY = level->worldY + inst->px_offset_y;

				// check bounds
				Rectangle instBounds = {
					.x = instWorldX,
					.y = instWorldY,
					.width = (inst->cWid * grid_size),
					.height = (inst->cHei * grid_size)
				};

				if (CheckCollisionRecs(rayBounds, instBounds))
				{
					// Walk along the ray, jumping to each cell boundary along the way
					// Algorithm implemented from here
					// http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html



					// transform world space ray into cell relative
					Vector2 rayStart = { (start.x - (float)instWorldX) / gridF, (start.y - (float)instWorldY) / gridF };
					Vector2 rayEnd = { (end.x - (float)instWorldX) / gridF, (end.y - (float)instWorldY) / gridF };

					float dx = rayEnd.x - rayStart.x;
					float dy = rayEnd.y - rayStart.y;
					float dt_dx = 1.0f / dx;
					float dt_dy = 1.0f / dy;

					int x = (int)floorf(rayStart.x);
					int y = (int)floorf(rayStart.y);
					int n = 1;

					int incX, incY;
					float nextX, nextY;
					float t = 0.0f;

					if (dx == 0.0f)
					{
						incX = 0;
						nextX = dt_dx;	// infinity
					}
					else if (rayEnd.x > rayStart.x)
					{
						incX = 1;
						n += (int)floorf(rayEnd.x) - x;
						nextX = (floorf(rayStart.x) + 1 - rayStart.x) * dt_dx;
					}
					else
					{
						incX = -1;
						n += x - (int)floorf(rayEnd.x);
						nextX = (rayStart.x - floorf(rayStart.x)) * dt_dx;
					}

					if (dy == 0.0f)
					{
						incY = 0.0f;
						nextY = dt_dy;	// infinity
					}
					else if (rayEnd.y > rayStart.y)
					{
						incY = 1;
						n += (int)floor(rayEnd.y) - y;
						nextY = (floor(rayStart.y) + 1 - rayStart.y) * dt_dy;
					}
					else
					{
						incY = -1;
						n += y - (int)floor(rayEnd.y);
						nextY = (rayStart.y - floorf(rayStart.y)) * dt_dy;
					}

					for (; n > 0; --n)
					{
						// ray might originate or terminate outside of the bounds so only check grid cell for valid cells.
						// This could be improved by fast forwarding to the first cell in bounds, and terminating early as 
						// soon as the ray leaves the bounds.
						if (x >= 0 && x < inst->cWid && y >= 0 && y < inst->cHei)
						{
							int cellIdx = x + y * inst->cWid;
							int cellValue = inst->int_grid[cellIdx];
							if (cellValue != 0)
							{
								// TODO handle if the ray starts intersecting... could either return that case
								// or scan forward for the next change of value? Maybe both.

								// We have a collision! Store if it is the closest collision.
								float dist = t * gridF;
								if (dist < result.dist)
								{
									result.hasHit = true;
									result.dist = t * gridF;
									result.gridValue = cellValue;
									result.hitPos = (Vector2){
										(rayStart.x + dx * t)* gridF + instWorldX,
										(rayStart.y + dy * t) * gridF + instWorldY
									};

									// TODO figure this out from which branch was taken last loop?
									result.hitNormal = (Vector2) {0, -1};
								}

								// TODO we could stop the loop here but useful for now for debug draw purposes
							}
						}

						if (bDebugDraw)
						{
							// Draw a rectangle around the current cell being checked
							int x1 = x * grid_size + instWorldX;
							int y1 = y * grid_size + instWorldY;
							DrawRectangleLines(x1, y1, grid_size, grid_size, WHITE);

							// Draw a small pink circle where the ray crosses a cell boundary
							float worldx = (rayStart.x + dx * t) * gridF + instWorldX;
							float worldy = (rayStart.y + dy * t) * gridF + instWorldY;
							DrawCircle(worldx, worldy, 2, PINK);
						}

						if (fabsf(nextY) < fabsf(nextX))
						{
							y += incY;
							t = fabsf(nextY);
							nextY += dt_dy;
						}
						else
						{
							x += incX;
							t = fabsf(nextX);
							nextX += dt_dx;
						}
					}
				}
			}
		}
	}

	return result;
}



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


void DrawLevels(int showDepth)
{
	// draw everything for now
	if (!gWorld) return;

	int count = ldtk_get_level_count(gWorld);
	for (int i = 0; i < count; ++i)
	{
		ldtk_level* level = ldtk_get_level(gWorld, i);

		// render layers back to front
		for (int j = level->layer_instances_count-1; j >= 0 ; --j)
		{
			ldtk_layer_instance* inst = &level->layer_instances[j];
			Texture* tex = NULL;

			if (level->worldDepth != showDepth)
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


typedef struct PlayerInput
{
	bool bMoveLeft;
	bool bMoveRight;
	bool bJump;
} PlayerInput;


PlayerInput GetPlayerInput()
{
	PlayerInput input;
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

typedef struct Player
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
} Player;


typedef struct GameState
{
	Player Player;
	PlayerInput Input;
} GameState;



//////////////////////////////////////////////////////////////////////////
// some play related config variables - where best to keep these? in a struct?

// player width
static int gPlayerWidth = 16;
// player height
static int gPlayerHeight = 24;
// max height of jump if player holds the jump button
static float gPlayerJumpHeight = 32.0f;
// how many frames to reach peak height?
static float gPlayerJumpFrames = 20;
// alternative to JumpFrames - pick a target distance (at max speed)
static float gPlayerJumpDistance = 180.0f;

// change jump gravity this much when falling (>1 for falling faster)
static float gPlayerFallGravityScale = 1.0f;

// change jump gravity this much when player releases jump early
static float gPlayerJumpReleaseGravityScale = 4.0f;

// how many frames to pause at the top of the jump
static int gPlayerHangTimeFrames = 3;

// 0 - no accel, 1 infinite accel
static float gPlayerAccelX = 0.8f;
// max speed pixels/frame
static float gPlayerMaxSpeedX = 2.0f;

// max speed pixels/frame
static float gPlayerMaxFallSpeed = 10.0f;

// 2 is double jump
static int gPlayerMaxJumpCount = 100;

// how many frames of coyote time
static int gPlayerCoyoteTime = 3;

// track how high the player has jumped
static float gStat_MaxHeight = 0.0f;


void UpdatePlayer(GameState* state)
{
	const PlayerInput* input = &state->Input;
	Player* player = &state->Player;


	// take off vertical speed
	const float v0 = (-2.0f * gPlayerJumpHeight * gPlayerMaxFallSpeed) / gPlayerJumpDistance;
	// gravity
	const float G = (2.0f * gPlayerJumpHeight * (gPlayerMaxFallSpeed * gPlayerMaxFallSpeed)) / (gPlayerJumpDistance * gPlayerJumpDistance);

	// transient downward force applied to the player (differs depending on certain conditions)
	float g = G;

	Vector2 posDelta = { 0 };

	//////////////////////////////////////////////////////////////////////////
	// horizontal movement
	float targetSpeedX = 0.0f;
	if (input->bMoveLeft)
	{
		targetSpeedX = -gPlayerMaxSpeedX;
	}
	else if (input->bMoveRight)
	{
		targetSpeedX = gPlayerMaxSpeedX;
	}

	// store desired delta of player position in X axis
	posDelta.x = gPlayerAccelX * targetSpeedX + (1.0f - gPlayerAccelX) * player->Velocity.x;
	player->Velocity.x = posDelta.x;


	//////////////////////////////////////////////////////////////////////////
	// vertical movement

	// TODO check if player is on the ground (raycast?)
	// for now collide with the y=0 plane
	bool bIsGrounded = player->Location.y >= 0.0f;

	//WorldTrace(gWorld, player->Location, Vector2Add(player->Location, (Vector2) { 0.0f, 100.0f }));

	if (bIsGrounded)
	{
		player->bIsJumping = false;
		player->JumpCount = 0;
		player->JumpState = PJS_None;
		player->JumpStateTime = 0;
		player->NotGroundedTime = 0;
		player->Velocity.y = 0.0f;
	}
	else
	{
		player->NotGroundedTime++;
		if (!player->bIsJumping && player->JumpState == 0)
		{
			// player walked off an edge?
			//player->JumpCount = 1;
			player->JumpState = PJS_Falling;
		}

		// after N frames clear coyote time by setting JumpCount to 1
		if (!player->bIsJumping && player->JumpState == 3 && player->JumpCount == 0)
		{
			if (player->NotGroundedTime > gPlayerCoyoteTime)
			{
				player->JumpCount = 1;
			}
		}
	}

	if (input->bJump && !player->bJumpPrev && player->JumpCount < gPlayerMaxJumpCount)
	{
		// start jumping
		player->bIsJumping = true;
		player->JumpState = PJS_JumpAscending;
		player->JumpStateTime = 0;
		player->JumpCount++;

		// apply initial impulse
		player->Velocity.y = v0;

		if (bIsGrounded)
		{
			// reset max height when we start jumping again
			gStat_MaxHeight = 0;
		}
	}

	switch (player->JumpState)
	{
		// no jump
	case PJS_None:
		break;

		// ascending
	case PJS_JumpAscending:
		if (player->Velocity.y > 0.0f)
		{
			// reached the peak
			player->Velocity.y = 0.0f;
			player->JumpState = PJS_JumpApex;
			player->JumpStateTime = 0;
		}

		// if the player releases the button on the way up, slow them down quicker
		if (!input->bJump)
		{
			g = G * gPlayerJumpReleaseGravityScale;
		}
		break;

		// hang time
	case PJS_JumpApex:
		player->Velocity.y = 0.0f;
		g = 0.0f;

		// wait a few frames then move to falling state
		if (player->JumpStateTime >= gPlayerHangTimeFrames)
		{
			player->JumpState = PJS_Falling;
			player->JumpStateTime = 0;
		}
		break;

		// falling
	case PJS_Falling:
		if (player->Velocity.y > 0.0f)
		{
			g = G * gPlayerFallGravityScale;
		}
		break;
	}

	player->JumpStateTime += 1;

	posDelta.y = player->Velocity.y;// + (g / 2.0f);
	player->Velocity.y += g;

	// clamp to max speed
	if (player->Velocity.y > gPlayerMaxFallSpeed)
	{
		player->Velocity.y = gPlayerMaxFallSpeed;
	}


	//////////////////////////////////////////////////////////////////////////
	// Move player according to desired delta (todo - collide with world!)
	player->Location = Vector2Add(player->Location, posDelta);


	// store if the player had Jump pressed this frame
	player->bJumpPrev = input->bJump;
}


//////////////////////////////////////////////////////////////////////////


static GameState StepGame(GameState state)
{
	// start by copying the previous gamestate
	GameState newState = state;

	// update player
	UpdatePlayer(&newState);

	// track some random stats...
	if (gStat_MaxHeight > newState.Player.Location.y)
	{
		gStat_MaxHeight = newState.Player.Location.y;
	}

	return newState;
}



//////////////////////////////////////////////////////////////////////////
// Main loop

typedef enum EStepMode
{
	StepMode_Play,
	StepMode_Paused,
	StepMode_Replay,
} EStepMode;


// game state tracking
static int gCurrentFrame = 0;
#define kMaxGameStates (60 * 60 * 10)	// 60 FPS * 60s * 10 = 10mins capacity (currently ~1.4mb)
static GameState gGameStates[kMaxGameStates] = {0};
static int gGameStateCount = 0;
static EStepMode gStepMode = StepMode_Play;
static bool gDebugUI_Timeline = false;


// Reset gamestates to initial conditions
static void InitGameState()
{
	memset(gGameStates, 0, sizeof(gGameStates[0]));
	gGameStates[0].Player.Location = (Vector2){ 24, -26 };

	gGameStateCount = 1;
	gCurrentFrame = 0;
}


static void DrawDebugUI()
{
	if (!gDebugUI_Timeline) return;

	char frameTxt[1024];
	float y = (float)GetScreenHeight() - 70;
	float x = (float)GetScreenWidth() / 2 - 512;
	float iw = 32;

	// jump to start
	if (GuiButton((Rectangle) { x, y, iw, iw }, "#129#"))
	{
		gStepMode = StepMode_Paused;
		gCurrentFrame = 0;
	}
	// step backwards
	if (GuiButton((Rectangle) { x += iw, y, iw, iw }, "#114#"))
	{
		gStepMode = StepMode_Paused;
		if (gCurrentFrame > 0)
		{
			gCurrentFrame--;
		}
	}
	// step forwards
	if (GuiButton((Rectangle) { x += iw, y, iw, iw }, "#115#"))
	{
		gStepMode = StepMode_Paused;
		if (gCurrentFrame < (gGameStateCount - 1))
		{
			gCurrentFrame++;
		}
		else
		{
			// create a new gamestate!
			GameState gameState = StepGame(gGameStates[gCurrentFrame]);

			if ((gCurrentFrame + 1) < kMaxGameStates)
			{
				gCurrentFrame++;
				gGameStates[gCurrentFrame] = gameState;
				gGameStateCount = gCurrentFrame + 1;
			}
		}
	}
	// pause
	if (GuiButton((Rectangle) { x += iw, y, iw, iw }, "#132#"))
	{
		gStepMode = StepMode_Paused;
	}
	// replay from current location
	if (GuiButton((Rectangle) { x += iw, y, iw, iw }, "#131#"))
	{
		gStepMode = StepMode_Replay;
	}
	// jump to end
	if (GuiButton((Rectangle) { x += iw, y, iw, iw }, "#134#"))
	{
		gStepMode = StepMode_Paused;
		gCurrentFrame = gGameStateCount - 1;
	}
	// resume gameplay from here
	if (GuiButton((Rectangle) { x += iw, y, iw, iw }, "#150#"))
	{
		gStepMode = StepMode_Play;
	}
	x += iw + 16;

	switch (gStepMode)
	{
	case StepMode_Play:	strcpy(frameTxt, "PLAYING");	break;
	case StepMode_Paused:	strcpy(frameTxt, "PAUSED");		break;
	case StepMode_Replay:	strcpy(frameTxt, "REPLAY");		break;
	default:	frameTxt[0] = 0;
	}
	GuiDrawText(frameTxt, (Rectangle) { x, y, 64, 32 }, TEXT_ALIGN_LEFT, RAYWHITE);
	x += 64;

	sprintf(frameTxt, "%d / %d", gCurrentFrame, gGameStateCount - 1);
	GuiDrawText(frameTxt, (Rectangle) { x, y, 64, 32 }, TEXT_ALIGN_LEFT, RAYWHITE);
	x += 64;

	// show player inputs
	PlayerInput* input = &gGameStates[gCurrentFrame].Input;
	x += 16;
	input->bJump = GuiCheckBox((Rectangle) { x, y, 16, 16 }, "Jump", input->bJump);
	y += 16;
	input->bMoveLeft = GuiCheckBox((Rectangle) { x, y, 16, 16 }, "Left", input->bMoveLeft);
	x += 48;
	input->bMoveRight = GuiCheckBox((Rectangle) { x, y, 16, 16 }, "Right", input->bMoveRight);

	// timeline slider allows scrubbing thru saved states
	{
		if (gStepMode == StepMode_Play)
		{
			// do not allow setting slider values if playing!
			GuiSetState(STATE_DISABLED);
		}

		sprintf(frameTxt, "%d", gGameStateCount - 1);
		float v = GuiSlider((Rectangle) { 128, (float)GetScreenHeight() - 32, (float)GetScreenWidth() - 256, 16 }, "0", frameTxt, (float)gCurrentFrame, 0.0f, (float)gGameStateCount);
		if ((int)v != gCurrentFrame && (int)v < gGameStateCount)
		{
			gCurrentFrame = (int)v;
		}

		GuiSetState(STATE_NORMAL);
	}
}


static void DrawPlayer(GameState state)
{
	int pw = gPlayerWidth;
	int ph = gPlayerHeight;
	int px = (int)state.Player.Location.x - (pw / 2);
	int py = (int)state.Player.Location.y - ph;
	DrawRectangle(px, py, pw, ph, RAYWHITE);
}


static void DrawTraceResult(TraceResult hit)
{
	// draw a line from start to end
	// draw a line from start to hit
	// draw an arrow showing normal

	// invoke the function with debug draw enabled
	WorldTrace(gWorld, hit.start, hit.end, true);

	if (hit.hasHit)
	{
		DrawLine(hit.start.x, hit.start.y, hit.hitPos.x, hit.hitPos.y, RED);
		DrawLine(hit.hitPos.x, hit.hitPos.y, hit.end.x, hit.end.y, YELLOW);
	}
	else
	{
		DrawLine(hit.start.x, hit.start.y, hit.end.x, hit.end.y, YELLOW);
	}
}


//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

static int framesCounter = 0;
static int finishScreen = 0;
static Vector2 cameraOffset = { 0 };
static float cameraZoom = 1.0f;
static int worldDepthToShow = 0;

static Camera2D currentCamera = {0};

static bool gMouseRightDown = false;
static Vector2 gMouseRayStart;
static TraceResult MouseRayHit = {0};

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
	// TODO: Initialize GAMEPLAY screen variables here!
	framesCounter = 0;
	finishScreen = 0;

	gWorld = ldtk_load_world("resources/WorldMap_GridVania_layout.ldtk");

	if (gWorld)
	{
		// load textures for tilesets and store pointer in tileset userdata
		char texturePath[260];
		int count = ldtk_get_tileset_count(gWorld);
		if (count > 16) count = 16;

		for (int i = 0; i < count; ++i)
		{
			struct ldtk_tileset* tileset = ldtk_get_tileset(gWorld, i);
			sprintf(texturePath, "resources/%s", tileset->relPath);

			if (strstr(texturePath, ".aseprite"))
			{
				gWorldSprites[i] = LoadAseprite(texturePath);
				gWorldTextures[i] = GetAsepriteTexture(gWorldSprites[i]);
				tileset->userdata = &gWorldTextures[i];
			}
			else
			{
				gWorldTextures[i] = LoadTexture(texturePath);
				tileset->userdata = &gWorldTextures[i];
			}
		}


		// test collisions!
		//WorldTrace(gWorld, (Vector2){ 128, 32 }, (Vector2) { 128, 512 });
		//WorldTrace(gWorld, (Vector2) { 40, 40 }, (Vector2) { 228, 40 });
	}

	// setup initial gamestate
	InitGameState();

	// init mouse ray to something known so we can debug it easily
	MouseRayHit = (TraceResult) {
		.start = (Vector2) {0, 0},
		.end = (Vector2) {127, 15}
	};
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

	currentCamera = (Camera2D){
		.offset = (Vector2) { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f },
		.target = (Vector2) { -cameraOffset.x, -cameraOffset.y },
		.zoom = cameraZoom
	};

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }

	if (IsKeyPressed(KEY_R))
	{
		InitGameState();
	}

	if (IsKeyPressed(KEY_F1))
	{
		gDebugUI_Timeline = !gDebugUI_Timeline;
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		if (!gMouseRightDown)
		{
			gMouseRayStart = GetMousePosition();
		}

		// do raycast!
		Vector2 start = GetScreenToWorld2D(gMouseRayStart, currentCamera);
		Vector2 end = GetScreenToWorld2D(GetMousePosition(), currentCamera);

		MouseRayHit = WorldTrace(gWorld, start, end, false);
	}
	gMouseRightDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

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


// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

	BeginMode2D(currentCamera);
		DrawLevels(worldDepthToShow);

		DrawPlayer(gGameStates[gCurrentFrame]);

		DrawTraceResult(MouseRayHit);
	EndMode2D();

	DrawDebugUI();

    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    //DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

	DrawFPS(GetScreenWidth() - 100, 10);
	DrawText(TextFormat("Height: %f", -gStat_MaxHeight), GetScreenWidth() - 200, 60, 10, RAYWHITE);
}


// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!

	for (int i = 0; i < 16; ++i)
	{
		UnloadTexture(gWorldTextures[i]);
		UnloadAseprite(gWorldSprites[i]);
	}

    ldtk_destroy_world(gWorld);
}


// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}

