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

#ifndef _SDLCONSOLE_H_
#define _SDLCONSOLE_H_

//#include <SDL.h>
#include <SDL2/SDL.h>
#define SDLCONSOLE_EVENT_NONE		0
#define SDLCONSOLE_EVENT_KEY		1
#define SDLCONSOLE_EVENT_QUIT		2
#define SDLCONSOLE_EVENT_DEBUG_1	3
#define SDLCONSOLE_EVENT_DEBUG_2	4

int sdlconsole_init(char *title);
void sdlconsole_blit(uint32_t* pixels, int w, int h, int stride);
int sdlconsole_loop();

uint32_t sdlconsole_getScanCode();
uint8_t sdlconsole_getModKeys();

int sdlconsole_setWindow(int w, int h);
void sdlconsole_setTitle(char* title);

#endif
