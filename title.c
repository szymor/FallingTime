/*
 * Ativayeban, title screen code file
 * Copyright (C) 2014 Nebuleon Fumika <nebuleon@gcw-zero.com>
 * 2015 Cong Xu <congusbongus@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "title.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>

#include "SDL_image.h"

#include "animation.h"
#include "block.h"
#include "main.h"
#include "init.h"
#include "input.h"
#include "platform.h"
#include "player.h"
#include "sound.h"
#include "space.h"
#include "text.h"
#include "game.h"
#include "bg.h"
#include "sys_specifics.h"

static bool Start = false;
static bool  WaitingForRelease = false;
static char WelcomeMessage[256];
static int winnerIndices[MAX_PLAYERS];
static int winners = 0;
static Animation TitleAnim;
static Animation GameOverAnim;
SDL_Surface *ControlSurfaces[MAX_PLAYERS];

static Block blocks[MAX_PLAYERS];
#define BLOCK_WIDTH (FIELD_WIDTH / MAX_PLAYERS * 0.25f)
#define BLOCK_Y (FIELD_HEIGHT * 0.5f)

// Countdown to start the game automatically; starts when a player is enabled
static int countdownMs = -1;
#define COUNTDOWN_START_MS 3999

bool playersEnabled[MAX_PLAYERS];


static void TitleScreenEnd(void);
void TitleScreenGatherInput(bool* Continue)
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev))
	{
		if (IsExitGameEvent(&ev))
		{
			*Continue = false;
			TitleScreenEnd();
			return;
		}
		InputOnEvent(&ev);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			players[i].AccelX = GetMovement(i);
		}
	}
}
static void TitleScreenEnd(void)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		BlockRemove(&blocks[i]);
		// Kill players that have not been enabled
		players[i].Enabled = playersEnabled[i];
		players[i].Alive = playersEnabled[i];
	}
}

void TitleScreenDoLogic(bool* Continue, bool* Error, Uint32 Milliseconds)
{
	(void)Continue;
	(void)Error;
	cpSpaceStep(space.Space, Milliseconds * 0.001);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerUpdate(&players[i], Milliseconds);

		// Check which players have fallen below their start pads
		cpVect pos = cpBodyGetPosition(players[i].Body);
		if (pos.y < BLOCK_Y)
		{
			if (!playersEnabled[i])
			{
				// New player entered
				countdownMs = COUNTDOWN_START_MS;
				SoundPlay(SoundStart, 1.0);
			}
			playersEnabled[i] = true;
		}
	}

	if (countdownMs >= 0)
	{
		const int countdownMsNext = countdownMs - Milliseconds;
		// Play a beep every second
		if ((countdownMs / 1000) > (countdownMsNext / 1000))
		{
			SoundPlay(SoundBeep, 1.0);
		}
		// Start game if counted down to zero
		if (countdownMsNext <= 0)
		{
			TitleScreenEnd();
			ToGame();
			return;
		}
		countdownMs = countdownMsNext;
	}

	Animation *a = Start ? &TitleAnim : &GameOverAnim;
	AnimationUpdate(a, Milliseconds);
}

static void DrawTitleImg(void);
void TitleScreenOutputFrame(void)
{
	DrawBackground(&BG, 0);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SDL_Rect dest =
		{
			(Sint16)(
				SCREEN_X((i + 1) * FIELD_WIDTH / (MAX_PLAYERS + 1)) -
				ControlSurfaces[i]->w / 2),
			(Sint16)(
				(SCREEN_HEIGHT - ControlSurfaces[i]->h) / 2 -
				SCREEN_X(PLAYER_RADIUS)),
			0,
			0
		};
		SDL_BlitSurface(ControlSurfaces[i], NULL, Screen, &dest);
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerDraw(&players[i], 0);
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		BlockDraw(&blocks[i], 0);
	}

	DrawTitleImg();
	// Draw player icons if winners
	if (!Start)
	{
		const int left =
			(SCREEN_WIDTH - PLAYER_SPRITESHEET_WIDTH * winners) / 2;
		for (int i = 0; i < winners; i++)
		{
			const int playerIndex = winnerIndices[i];
			SDL_Rect src = {
				0, 0, PLAYER_SPRITESHEET_WIDTH, PLAYER_SPRITESHEET_HEIGHT
			};
			SDL_Rect dest =
			{
				(Sint16)(left + i * PLAYER_SPRITESHEET_WIDTH),
				(Sint16)(SCREEN_HEIGHT * 0.66f),
				0, 0
			};
			SDL_BlitSurface(
				PlayerSpritesheets[playerIndex], &src, Screen, &dest);
		}
	}
	TextRenderCentered(
		Screen, font, WelcomeMessage, (int)(SCREEN_HEIGHT * 0.75f));

	SDL_Flip(Screen);
}
static void DrawTitleImg(void)
{
	const Animation *a = Start ? &TitleAnim : &GameOverAnim;
	AnimationDrawUpperCenter(a, Screen);
}

void ToTitleScreen(const bool start)
{
	countdownMs = -1;
	ResetMovement();
	MusicSetLoud(false);
	BackgroundsInit(&BG);
	Start = start;
	if (Start)
	{
		sprintf(
			WelcomeMessage,
			"%s to pause\n%s to exit",
			GetPausePrompt(), GetExitGamePrompt());
	}
	else
	{
		// Find out the result of the game
		int maxScore = 0;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (players[i].Score > maxScore) maxScore = players[i].Score;
		}
		winners = 0;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (!players[i].Enabled) continue;
			if (players[i].Score == maxScore)
			{
				winnerIndices[winners] = i;
				winners++;
			}
		}
		if (PlayerEnabledCount() == 1)
		{
			sprintf(
				WelcomeMessage,
				"Your score was %d!\n%s to exit",
				maxScore, GetExitGamePrompt());
		}
		else if (winners == 1)
		{
			sprintf(
				WelcomeMessage,
				"Wins with score %d!\n%s to exit",
				maxScore, GetExitGamePrompt());
		}
		else
		{
			sprintf(
				WelcomeMessage,
				"Tied with score %d!\n%s to exit",
				maxScore, GetExitGamePrompt());
		}
	}

	SpaceReset(&space);
	// Add bottom edge so we don't fall through
	SpaceAddBottomEdge(&space);

	// Initialise players here
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerInit(&players[i], i, cpv(
			(i + 1) * FIELD_WIDTH / (MAX_PLAYERS + 1),
			FIELD_HEIGHT * 0.75f));
		playersEnabled[i] = false;
	}

	// Add platforms for players to jump off
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		BlockInit(
			&blocks[i],
			(i + 1) * FIELD_WIDTH / (MAX_PLAYERS + 1) - BLOCK_WIDTH / 2,
			BLOCK_Y,
			BLOCK_WIDTH);
	}

	GatherInput = TitleScreenGatherInput;
	DoLogic     = TitleScreenDoLogic;
	OutputFrame = TitleScreenOutputFrame;
}

bool TitleImagesLoad(void)
{
	if (!AnimationLoad(&TitleAnim, "data/graphics/anim%02d.png", 12, 12))
	{
		return false;
	}
	if (!AnimationLoad(&GameOverAnim, "data/graphics/gameover_%02d.png", 2, 3))
	{
		return false;
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		char buf[256];
#ifdef __GCW0__
		sprintf(buf, "data/graphics/gcw%d.png", i);
#else
		sprintf(buf, "data/graphics/keyboard%d.png", i);
#endif
		ControlSurfaces[i] = IMG_Load(buf);
		if (ControlSurfaces[i] == NULL)
		{
			return false;
		}
	}
	return true;
}
void TitleImagesFree(void)
{
	AnimationFree(&TitleAnim);
	AnimationFree(&GameOverAnim);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SDL_FreeSurface(ControlSurfaces[i]);
	}
}
