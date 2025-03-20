/*
GRiD Compass emulator
Copyright (C)2022 JDat
https://github.com/JDat/GRiDemulator

Based on XTulator: A portable, open-source 80186 PC emulator.
Copyright (C)2020 Mike Chambers
https://github.com/mikechambers84/XTulator

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "config.h"

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdint.h>
#include "sdlconsole.h"
#include "timing.h"
#include "debuglog.h"
#include "utility.h"

#include <stdbool.h>

SDL_Window *sdlconsole_window = NULL;
SDL_Renderer *sdlconsole_renderer = NULL;
SDL_Texture *sdlconsole_texture = NULL;

uint64_t sdlconsole_frameTime[30];
uint32_t sdlconsole_keyTimer;
uint32_t sdlconsole_curkey;
uint8_t sdlconsole_modKeys;
uint32_t sdlconsole_lastKey;
uint8_t sdlconsole_frameIdx = 0, sdlconsole_grabbed = 0, sdlconsole_doRepeat = 0;

int sdlconsole_curw, sdlconsole_curh;

char* sdlconsole_title;

int sdlconsole_init(char *title) {

  if (SDL_Init(SDL_INIT_VIDEO)) return -1;

  sdlconsole_title = title;

  sdlconsole_window = SDL_CreateWindow(sdlconsole_title,
  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
  screenWidth, screenHeight,    // window size = GRiD screen size
  SDL_WINDOW_OPENGL);
  if (sdlconsole_window == NULL) return -1;

  //if (sdlconsole_setWindow(640, 400)) {
  if (sdlconsole_setWindow(screenWidth, screenHeight)) {
    return -1;
  }

return 0;
}

int sdlconsole_setWindow(int w, int h) {
  
  if (sdlconsole_renderer != NULL) SDL_DestroyRenderer(sdlconsole_renderer);
  if (sdlconsole_texture != NULL) SDL_DestroyTexture(sdlconsole_texture);
  sdlconsole_renderer = NULL;
  sdlconsole_texture = NULL;

  SDL_SetWindowSize(sdlconsole_window, w, h);
   
  sdlconsole_renderer = SDL_CreateRenderer(sdlconsole_window, -1, 0);
  if (sdlconsole_renderer == NULL) return -1;

  sdlconsole_texture = SDL_CreateTexture(sdlconsole_renderer,
  SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
  if (sdlconsole_texture == NULL) return -1;

  sdlconsole_curw = w;
  sdlconsole_curh = h;

  return 0;
}

void sdlconsole_setTitle(char* title) { //appends something to the main title, doesn't replace it all
  char tmp[1024];
  sprintf(tmp, "%s - %s", sdlconsole_title, title);
  SDL_SetWindowTitle(sdlconsole_window, tmp);
}

void sdlconsole_blit(uint32_t *pixels, int w, int h, int stride) {
  static uint64_t lasttime = 0;
  uint64_t curtime;
  //SDL_Rect rect;
  curtime = timing_getCur();

  if ((w != sdlconsole_curw) || (h != sdlconsole_curh)) {
    sdlconsole_setWindow(w, h);
  }

  SDL_UpdateTexture(sdlconsole_texture, NULL, pixels, stride);
  SDL_RenderClear(sdlconsole_renderer);
  SDL_RenderCopy(sdlconsole_renderer, sdlconsole_texture, NULL, NULL);
  SDL_RenderPresent(sdlconsole_renderer);

  if (lasttime != 0) {
    int i, avgcount;
    uint64_t curavg;
    char tmp[64];
    sdlconsole_frameTime[sdlconsole_frameIdx++] = curtime - lasttime;
    if (sdlconsole_frameIdx == 30) {
      sdlconsole_frameIdx = 0;
      avgcount = 0;
      curavg = 0;
      for (i = 0; i < 30; i++) {
        if (sdlconsole_frameTime[i] != 0) {
          curavg += sdlconsole_frameTime[i];
          avgcount++;
        }
      }
      curavg /= avgcount;
      sprintf(tmp, "%.2f FPS", (double)((timing_getFreq() * 10) / curavg) / 10);
      sdlconsole_setTitle(tmp);
    }
  }
  lasttime = curtime;
}

int sdlconsole_loop() {
  SDL_Event event;

  if (!SDL_PollEvent(&event)) return SDLCONSOLE_EVENT_NONE;
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.repeat) return SDLCONSOLE_EVENT_NONE;
      switch (event.key.keysym.sym) {
        case SDLK_F1:
          return SDLCONSOLE_EVENT_DEBUG_1;
        case SDLK_F2:
          return SDLCONSOLE_EVENT_DEBUG_2;
        default:
        if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL) {
          bitSet(sdlconsole_modKeys, 6);
        }
        if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT) {
          bitSet(sdlconsole_modKeys, 5);
        }
        if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT) {
          bitSet(sdlconsole_modKeys, 4);
        }
        bitSet(sdlconsole_modKeys, 7);

        sdlconsole_curkey = event.key.keysym.sym;
        return SDLCONSOLE_EVENT_KEY;
      }
    case SDL_KEYUP:
      if (event.key.repeat) return SDLCONSOLE_EVENT_NONE;
      if (event.key.keysym.sym == SDLK_LCTRL) {
        bitClear(sdlconsole_modKeys, 6);
      }
      if (event.key.keysym.sym == SDLK_LALT) {
        bitClear(sdlconsole_modKeys, 5);
      }
      if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT) {
        bitClear(sdlconsole_modKeys, 4);
      }
      bitClear(sdlconsole_modKeys, 7);
      sdlconsole_curkey = event.key.keysym.sym;
      return SDLCONSOLE_EVENT_KEY;
    case SDL_QUIT:
      return SDLCONSOLE_EVENT_QUIT;
  }
  return SDLCONSOLE_EVENT_NONE;
}

uint32_t sdlconsole_getScanCode() {
  return sdlconsole_curkey;
}
uint8_t sdlconsole_getModKeys() {
  return sdlconsole_modKeys;
}
