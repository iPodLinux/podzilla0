/*
 * Steroids - just another asteroids clone 
 * Copyright (C) 2004  Fredrik Bergholtz
 *
 * Loosely based on the init code of bluecube by Sebastian Falbesoner.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "../pz.h"

#include "globals.h"
#include "grafix.h"

#include "ship.h"
#include "asteroid.h"
#include "shot.h"

static void steroids_NewGame(void);
//static void steroids_MainMenu_Loop(void);
static void steroids_Game_Loop(void);
static void steroids_DrawScene(void);
void steroids_StartGameOverAnimation(void);
static void steroids_GameOverAnimation(void);

void steroids_ClearRect(int x, int y, int w, int h);


Steroids_Globals  steroids_globals;
Steroids_Ship     steroids_ship;
Steroids_Asteroid steroids_asteroid[STEROIDS_ASTEROID_NUM];
Steroids_Shot     steroids_shotShip[STEROIDS_SHOT_NUM];
Steroids_Shot     steroids_shotUFO;

static void steroids_do_draw()
{
	pz_draw_header("Steroids");
}

static int steroids_handle_event (GR_EVENT *event)
{
    switch (steroids_globals.gameState)
    {
    case STEROIDS_GAME_STATE_PLAY:
	switch (event->type)
	{
	case GR_EVENT_TYPE_TIMER:
	    steroids_Game_Loop();
	    break;

	case GR_EVENT_TYPE_KEY_DOWN:
	    switch (event->keystroke.ch)
	    {
	    case '\r': // Wheel button
		// Fire:
		steroids_shot_newShip (steroids_shotShip,
				       &steroids_ship);
		break;

	    case 'd': // Play/pause button
		steroids_globals.gameState = STEROIDS_GAME_STATE_PAUSE;
		break;

	    case 'w': // Rewind button
		break;

	    case 'f': // Fast forward button
		// Engine thrust:
		steroids_ship_thrustOn (&steroids_ship);
		break;

	    case 'l': // Wheel left
		// Rotate ship clockwise
		steroids_ship_rotate (-STEROIDS_SHIP_ROTATION, &steroids_ship);
		break;

	    case 'r': // Wheel right
		// Rotate ship counter clockwise
		steroids_ship_rotate (STEROIDS_SHIP_ROTATION, &steroids_ship);
		break;

	    case 'm': // Menu button
		steroids_globals.gameState = STEROIDS_GAME_STATE_EXIT;
		break;

	    default:
		break;
	    } // keystroke
	    break;   // key down
/*
	case GR_EVENT_TYPE_KEY_UP:
	    switch (event->keystroke.ch)
	    {
	    case 'd': // Play/pause button
		printf ("play up\n");
		break;
	    } // keystroke
	    break;   // key up
*/
	} // event type
	break; // GAME_STATE_PLAY



    case STEROIDS_GAME_STATE_PAUSE:
	switch (event->type)
	{
	case GR_EVENT_TYPE_KEY_DOWN:
	    switch (event->keystroke.ch)
	    {
	    case 'd': // Play/pause button
		steroids_globals.gameState = STEROIDS_GAME_STATE_PLAY;
		break;

	    case 'm': // Menu button
		steroids_globals.gameState = STEROIDS_GAME_STATE_EXIT;
		break;
	    }
	    break;
	}
	break;
	


    case STEROIDS_GAME_STATE_GAMEOVER:
	steroids_globals.gameState = STEROIDS_GAME_STATE_EXIT;
	break;


    case STEROIDS_GAME_STATE_EXIT:
	pz_close_window(steroids_globals.wid);
	GrDestroyTimer(steroids_globals.timer_id);
	break;
    }




    return 1;
}

void new_steroids_window(void)
{
	/* Init randomizer */
	srand(time(NULL));

	GrGetScreenInfo(&steroids_globals.screen_info);

	steroids_globals.width = steroids_globals.screen_info.cols;
	steroids_globals.height = steroids_globals.screen_info.rows
	                          - (HEADER_TOPLINE + 1);

	steroids_globals.gc = GrNewGC();
	GrSetGCUseBackground(steroids_globals.gc, GR_TRUE);

	steroids_globals.wid = pz_new_window(0,
				     HEADER_TOPLINE + 1,
				     steroids_globals.width,
				     steroids_globals.height,
				     steroids_do_draw,
				     steroids_handle_event);
	GrSelectEvents(steroids_globals.wid,
		         GR_EVENT_MASK_EXPOSURE
		       | GR_EVENT_MASK_KEY_DOWN
		       | GR_EVENT_MASK_KEY_UP
		       | GR_EVENT_MASK_TIMER);
	GrMapWindow(steroids_globals.wid);

	/* intialise game state */
	steroids_NewGame();

	steroids_globals.timer_id = GrCreateTimer(steroids_globals.wid, 150);
}



#ifdef USE_SDL
/*=========================================================================
// Name: TimeLeft()
// Desc: Returns the number of ms to wait that the framerate is constant
//=======================================================================*/
Uint32 TimeLeft()
{
	static Uint32 next_time=0;
	Uint32 now;

	now = SDL_GetTicks();
	if (next_time <= now) {
		next_time = now + TICK_INTERVAL;
		return 0;
	}

	return (next_time - now);
}
#endif


/*=========================================================================
// Name: NewGame()
// Desc: Starts a new game
//=======================================================================*/
static void steroids_NewGame()
{
    steroids_asteroid_initall (steroids_asteroid);
    steroids_ship_init (&steroids_ship);

    steroids_globals.score = 0; /* Reset score */
    steroids_globals.level = 0; /* Reset level */

    steroids_globals.gameState = STEROIDS_GAME_STATE_PLAY; /* Set playstate */
}


/*=========================================================================
// Name: Game_Loop()
// Desc: The main loop for the game
//=======================================================================*/
static void steroids_Game_Loop()
{
    int collide;
    int i;

    switch (steroids_globals.gameState)
    {
    case STEROIDS_GAME_STATE_PLAY:
	// Animate
	steroids_asteroid_animateall (steroids_asteroid);
	steroids_shot_animateall (steroids_shotShip);
	steroids_ship_animate (&steroids_ship);

	// Check ship collisions:
	collide = steroids_asteroid_collideShip (steroids_asteroid,
						 &steroids_ship);
// My own shots are lethal:
//      collide |= steroids_shot_collideShip (steroids_shot,
//    					  &steroids_ship);
	if (collide)
	{
	    // TODO: Explode ship
	    // TODO: Subtract "lives"
	    // TODO: Check for game over
	}

	// Check asteroid collisions:
	collide = steroids_asteroid_collideShot (steroids_asteroid,
						 steroids_shotShip);
	if (collide)
	{
	    i = 0;
	    while (i < STEROIDS_ASTEROID_NUM
		   && !steroids_asteroid[i].active)
	    {
		i++;
	    }
	    if (i == STEROIDS_ASTEROID_NUM)
	    {
		// TODO: Increase level.
		steroids_asteroid_initall (steroids_asteroid);
	    }
	}
	break;

    case STEROIDS_GAME_STATE_GAMEOVER:
	steroids_GameOverAnimation();
	break;
    }

    steroids_DrawScene();
}

/*=========================================================================
// Name: DrawScene()
// Desc: Draws the whole scene!
//=======================================================================*/
static void steroids_DrawScene()
{
    char chScore[30];
    char chLevel[30];
//    int i;

    switch (steroids_globals.gameState)
    {
    case STEROIDS_GAME_STATE_PLAY:
	// Clear playfield:
	//
	steroids_ClearRect (0,
			    0,
			    steroids_globals.width,
			    steroids_globals.height);
/*
	chScore[0] = '\0';
	for (i = 0; i < STEROIDS_SHOT_NUM; i++)
	{
	    sprintf (chScore, "%s%d", chScore, steroids_shotShip[i].active);
	}
	GrText(steroids_globals.wid,
	       steroids_globals.gc,
	       1, 103,
	       chScore,
	       -1,
	       GR_TFASCII);
*/
/*
	sprintf (chScore, "%.2f, %.2f",
		 steroids_asteroid[0].shape.pos.x,
		 steroids_asteroid[0].shape.pos.y);
	GrText(steroids_globals.wid,
	       steroids_globals.gc,
	       1, 90,
	       chScore,
	       -1,
	       GR_TFASCII);
	sprintf (chScore, "%d, %d",
		 steroids_ship.shape.geometry.polygon.cog.x,
		 steroids_ship.shape.geometry.polygon.cog.y);
	GrText(steroids_globals.wid,
	       steroids_globals.gc,
	       1, 103,
	       chScore,
	       -1,
	       GR_TFASCII);
*/
	steroids_asteroid_drawall (steroids_asteroid);
	steroids_shot_drawall (steroids_shotShip);
	steroids_ship_draw (&steroids_ship);
	break;


    case STEROIDS_GAME_STATE_GAMEOVER:
	GrSetGCForeground(steroids_globals.gc, WHITE);
	GrFillRect(steroids_globals.wid, steroids_globals.gc, 0, 0, 168, 128);
	GrSetGCForeground(steroids_globals.gc, BLACK);
	GrText(steroids_globals.wid, steroids_globals.gc,
	       45, 35,
	       "- Game Over -",
	       -1, GR_TFASCII);
	sprintf(chScore, "Your Score: %d", steroids_globals.score);
	GrText(steroids_globals.wid, steroids_globals.gc,
	       35, 65,
	       chScore,
	       -1,
	       GR_TFASCII);

	sprintf(chLevel, "Your Level: %d", steroids_globals.level);
	GrText(steroids_globals.wid, steroids_globals.gc,
	       35, 95,
	       chLevel,
	       -1,
	       GR_TFASCII);
	break;
    }
}

/*=========================================================================
// Name: StartGameOverAnimation()
// Desc: Starts the gameover animation
//=======================================================================*/
void steroids_StartGameOverAnimation()
{
    steroids_globals.gameOver = 1;
}

/*=========================================================================
// Name: GameOverAnimation()
// Desc: Gameover Animation!
//=======================================================================*/
static void steroids_GameOverAnimation()
{
}

/*=========================================================================
// Name: PutRect()
// Desc: Draws a /UN/ filled rectangle onto the screen
//=======================================================================*/
void steroids_PutRect(int x, int y, int w, int h, int r, int g, int b)
{
    // GrSetGCForeground(steroids_globals.gc, MWRGB(r, g, b));
    GrSetGCForeground(steroids_globals.gc, BLACK);
    GrRect(steroids_globals.wid, steroids_globals.gc, x, y, w, h);
}

void steroids_ClearRect(int x, int y, int w, int h)
{
    GrSetGCForeground(steroids_globals.gc, WHITE);
    GrFillRect(steroids_globals.wid, steroids_globals.gc, x, y, w, h);
    GrSetGCForeground(steroids_globals.gc, BLACK);
}

