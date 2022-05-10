/*
 * Ativayeban, initialisation code file
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

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"
#include "SDL_image.h"

#include "gap.h"
#include "game.h"
#include "main.h"
#include "high_score.h"
#include "init.h"
#include "input.h"
#include "particle.h"
#include "pickup.h"
#include "platform.h"
#include "player.h"
#include "space.h"
#include "sound.h"
#include "title.h"

SDL_Surface *icon = NULL;

void Initialize(bool* Continue, bool* Error)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		*Continue = false;  *Error = true;
		printf("SDL initialisation failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL initialisation succeeded\n");

	Screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

	if (Screen == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_SetVideoMode succeeded\n");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
	{
		*Continue = false;  *Error = true;
		printf("Mix_OpenAudio failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("Mix_OpenAudio succeeded\n");

	InputInit();
	HighScoresInit();

	if (TTF_Init() == -1)
	{
		*Continue = false;  *Error = true;
		printf("TTF_Init failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("TTF_Init succeeded\n");

	SDL_WM_SetCaption("FallingTime", NULL);

#define LOAD_IMG(_surface, _path)\
	_surface = IMG_Load("data/graphics/" _path);\
	if (_surface == NULL)\
	{\
		*Continue = false;  *Error = true;\
		printf("IMG_Load failed: %s\n", SDL_GetError());\
		SDL_ClearError();\
		return;\
	}
	LOAD_IMG(icon, "icon.png");
	SDL_WM_SetIcon(icon, NULL);

	LOAD_IMG(PlayerSpritesheets[0], "penguin_ball.png");
	LOAD_IMG(PlayerSpritesheets[1], "penguin_black.png");
	LOAD_IMG(PickupImage, "eggplant.png");

#define LOAD_ANIM(_anim, _path, _w, _h, _fps)\
	if (!AnimationLoad(&(_anim), "data/graphics/" _path, (_w), (_h), (_fps)))\
	{\
		*Continue = false;  *Error = true;\
		printf("IMG_Load failed: %s\n", SDL_GetError());\
		SDL_ClearError();\
		return;\
	}
	LOAD_ANIM(Spark, "sparks.png", 4, 4, 20);
	LOAD_ANIM(SparkRed, "sparks_red.png", 4, 4, 20);
	LOAD_ANIM(Tail, "tail.png", 21, 21, 15);

	if (!GapSurfacesLoad())
	{
		*Continue = false;  *Error = true;
		printf("IMG_Load failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	if (!TitleImagesLoad())
	{
		*Continue = false;  *Error = true;
		printf("IMG_Load failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	if (!BackgroundsLoad(&BG))
	{
		*Continue = false;  *Error = true;
		printf("IMG_Load failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}

#define LOAD_SOUND(_sound, _path)\
	_sound = Mix_LoadWAV("data/sounds/" _path);\
	if (_sound == NULL)\
	{\
		*Continue = false;  *Error = true;\
		printf("Mix_LoadWAV failed: %s\n", SDL_GetError());\
		SDL_ClearError();\
		return;\
	}
	LOAD_SOUND(SoundBeep, "beep.ogg");
	LOAD_SOUND(SoundPlayerBounce, "bounce.ogg");
	LOAD_SOUND(SoundStart, "start.ogg");
	LOAD_SOUND(SoundLose, "lose.ogg");
	LOAD_SOUND(SoundScore, "score.ogg");
	SoundLoad();

	music = Mix_LoadMUS("data/sounds/music.ogg");
	if (music == NULL)
	{
		*Continue = false;  *Error = true;
		printf("Mix_LoadMUS failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}

#define LOAD_FONT(_f, _file, _size)\
	_f = TTF_OpenFont("data/" _file, _size);\
	if (_f == NULL)\
	{\
		*Continue = false;  *Error = true;\
		printf("TTF_OpenFont failed: %s\n", SDL_GetError());\
		SDL_ClearError();\
		return;\
	}
	LOAD_FONT(font, "LondrinaSolid-Regular.otf", 20);
	LOAD_FONT(hsFont, "LondrinaSolid-Regular.otf", 16);

	SpaceInit(&space);
	ParticlesInit();
	PickupsInit();

	SDL_ShowCursor(0);

	InitializePlatform();

	// Title screen. (-> title.c)
	MusicSetLoud(false);
	Mix_PlayMusic(music, -1);
	ToTitleScreen(true);
}

void Finalize()
{
	PickupsFree();
	ParticlesFree();
	SpaceFree(&space);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SDL_FreeSurface(PlayerSpritesheets[i]);
	}
	SDL_FreeSurface(PickupImage);
	SDL_FreeSurface(icon);
	AnimationFree(&Spark);
	AnimationFree(&SparkRed);
	AnimationFree(&Tail);
	GapSurfacesFree();
	TitleImagesFree();
	BackgroundsFree(&BG);
	Mix_FreeChunk(SoundPlayerBounce);
	Mix_FreeChunk(SoundBeep);
	Mix_FreeChunk(SoundStart);
	Mix_FreeChunk(SoundLose);
	Mix_FreeChunk(SoundScore);
	Mix_FreeMusic(music);
	SoundFree();
	TTF_CloseFont(font);
	TTF_CloseFont(hsFont);
	HighScoresFree();
	InputFree();
	SDL_Quit();
}
