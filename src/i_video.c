// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <unistd.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>

// #include <X11/Xlib.h>
// #include <X11/Xutil.h>
// #include <X11/keysym.h>

// #include <X11/extensions/XShm.h>
// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the XFree86 headers.
#ifdef LINUX
//int XShmGetEventBase( Display* dpy ); // problems with g++?
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
// #include <sys/socket.h>

// #include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "SDL3/SDL.h"

int SDLDoomKey(SDL_Keycode keycode);
int SDLKeycodeToASCII(SDL_Keycode keycode, SDL_Keymod keymod);

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

SDL_Color palette[256];

byte* mainScreen;

void I_InitGraphics(void) {
	if (!SDL_Init(SDL_INIT_VIDEO))
		I_Error("SDL_Init failed: %s", SDL_GetError());
	
	window = SDL_CreateWindow("wDOOM", SCREENWIDTH, SCREENHEIGHT, SDL_WINDOW_FULLSCREEN);

	if (!window)
		I_Error("Window creation failed: %s", SDL_GetError());

	renderer = SDL_CreateRenderer(window, NULL);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREENWIDTH, SCREENHEIGHT);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

	mainScreen = screens[0]; // bind screens[0] to the main screen
}

void I_ShutdownGraphics(void) {
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	mainScreen = NULL;
}

void I_SetPalette (byte* doomPalette) {
	for (int i = 0; i < 256; i++) {
		palette[i].r = gammatable[usegamma][*doomPalette++];
		palette[i].g = gammatable[usegamma][*doomPalette++];
		palette[i].b = gammatable[usegamma][*doomPalette++];
		palette[i].a = 255;
	}
}

void I_UpdateNoBlit (void) {
	//printf("Update No Blit\n");
}

void I_FinishUpdate (void) {
	void* pixels;
	int pitch;
	SDL_LockTexture(texture, NULL, &pixels, &pitch);

	uint32_t *dest = (uint32_t*)pixels;

	for (int y = 0; y < SCREENHEIGHT; y++) {
		for (int x = 0; x < SCREENWIDTH; x++) {
			byte colorIndex = mainScreen[y*SCREENWIDTH+x];
			SDL_Color c = palette[colorIndex];

			dest[y*(pitch/4)+x] = (c.r<<24) | (c.g<<16) | (c.b<<8) | 0xFF;
		}
	}

	SDL_UnlockTexture(texture);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void I_WaitVBL(int count) {
	SDL_Delay(1000*count/35);
}

void I_ReadScreen(byte *scr) {
	memcpy (scr, mainScreen, SCREENWIDTH*SCREENHEIGHT);
}

void I_BeginRead(void) {
	//printf("Begin Read\n");
}

void I_EndRead(void) {
	//printf("End Read\n");
}

void I_StartFrame(void) {
	SDL_Event e;
	event_t d;
	boolean valid = false;
	
	while (SDL_PollEvent(&e)) {
		switch(e.type) {
			case SDL_EVENT_QUIT:
				I_Quit();
				break;
			case SDL_EVENT_KEY_DOWN:
				d.type = ev_keydown;
				d.data1 = SDLDoomKey(SDL_GetKeyFromScancode(e.key.scancode, SDL_KMOD_NONE, false));
				valid = d.data1 > 0;
				if (!valid)
					d.data1 = SDLKeycodeToASCII(e.key.scancode, SDL_KMOD_NONE);
				break;
			case SDL_EVENT_KEY_UP:
				d.type = ev_keyup;
				d.data1 = SDLDoomKey(SDL_GetKeyFromScancode(e.key.scancode, SDL_KMOD_NONE, false));
				valid = d.data1 > 0;
				if (!valid)
					d.data1 = SDLKeycodeToASCII(e.key.scancode, SDL_KMOD_NONE);
				break;
			case SDL_EVENT_MOUSE_MOTION: // needs better mouse implementation
				// d.type = ev_mouse;
				// d.data1 = 0;
				// d.data2 = e.motion.xrel;
				// d.data3 = -e.motion.yrel;
				// did = true;
				break;
		}
		
		// only post event if it was valid
		if (valid) {
			D_PostEvent(&d);
			valid = false;
		}
	}
}

int SDLDoomKey(SDL_Keycode keycode) {
	int dcode;
	switch (keycode) {
		case SDLK_RIGHT: dcode = KEY_RIGHTARROW; break;
		case SDLK_LEFT: dcode = KEY_LEFTARROW; break;
		case SDLK_UP: dcode = KEY_UPARROW; break;
		case SDLK_DOWN: dcode = KEY_DOWNARROW; break;

		case SDLK_ESCAPE: dcode = KEY_ESCAPE; break;
		case SDLK_RETURN: dcode = KEY_ENTER; break;
		case SDLK_TAB: dcode = KEY_TAB; break;

		case SDLK_F1: dcode = KEY_F1; break;
		case SDLK_F2: dcode = KEY_F2; break;
		case SDLK_F3: dcode = KEY_F3; break;
		case SDLK_F4: dcode = KEY_F4; break;
		case SDLK_F5: dcode = KEY_F5; break;
		case SDLK_F6: dcode = KEY_F6; break;
		case SDLK_F7: dcode = KEY_F7; break;
		case SDLK_F8: dcode = KEY_F8; break;
		case SDLK_F9: dcode = KEY_F9; break;
		case SDLK_F10: dcode = KEY_F10; break;
		case SDLK_F11: dcode = KEY_F11; break;
		case SDLK_F12: dcode = KEY_F12; break;

		case SDLK_BACKSPACE: dcode = KEY_BACKSPACE; break;
		case SDLK_PAUSE: dcode = KEY_PAUSE; break;

		case SDLK_EQUALS: dcode = KEY_EQUALS; break;
		case SDLK_MINUS: dcode = KEY_MINUS; break;

		case SDLK_RSHIFT: dcode = KEY_RSHIFT; break;
		case SDLK_RCTRL: dcode = KEY_RCTRL; break;
		case SDLK_RALT: dcode = KEY_RALT; break;
		case SDLK_LALT: dcode = KEY_LALT; break;
		
		default: dcode = 0; break; // no valid input
	}
	return dcode;
}

int SDLKeycodeToASCII(SDL_Keycode keycode, SDL_Keymod keymod) {
    // Letters
    if (keycode >= SDLK_A && keycode <= SDLK_Z) {
        char base = 'a' + (keycode - SDLK_A);
        if (keymod & SDL_KMOD_SHIFT) {
            // uppercase if shift pressed
            base = base - 'a' + 'A';
        }
        return base;
    }
    // Digits (top row)
    if (keycode >= SDLK_0 && keycode <= SDLK_9) {
        // This ignores shift, so no symbols like ')', '!' handled here
        return '0' + (keycode - SDLK_0);
    }

    // Simple symbols (without shift)
    switch (keycode) {
        case SDLK_SPACE: return ' ';
        case SDLK_MINUS: return '-';
        case SDLK_EQUALS: return '=';
        case SDLK_LEFTBRACKET: return '[';
        case SDLK_RIGHTBRACKET: return ']';
        case SDLK_BACKSLASH: return '\\';
        case SDLK_SEMICOLON: return ';';
        case SDLK_APOSTROPHE: return '\'';
        case SDLK_COMMA: return ',';
        case SDLK_PERIOD: return '.';
        case SDLK_SLASH: return '/';
        case SDLK_GRAVE: return '`';
        default: return 0;  // no valid ascii
    }
}


void I_StartTic(void) {
	//printf("Start Tic\n");
}
