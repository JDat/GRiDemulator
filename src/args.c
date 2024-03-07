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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "timing.h"
#include "machine.h"
#include "cpu.h"
#include "i8259.h"
#include "debuglog.h"

double speedarg = 0;

int args_isMatch(char* s1, char* s2) {
  int i = 0, match = 1;

  while (1) {
    char c1, c2;
    c1 = s1[i];
    c2 = s2[i++];
    if ((c1 >= 'A') && (c1 <= 'Z')) {
      c1 -= 'A' - 'a';
    }
    if ((c2 >= 'A') && (c2 <= 'Z')) {
      c2 -= 'A' - 'a';
    }
    if (c1 != c2) {
      match = 0;
      break;
    }
      if (!c1 || !c2) {
      break;
    }
  }

  return match;
}

void args_showHelp() {
        printf(STR_TITLE " command line parameters:\r\n\r\n");

        printf("Machine options:\r\n");
        printf("  -machine <id>          Emulate machine definition defined by <id>. (Default is 1101)\r\n");
        printf("                         Use -machine list to display <id> options.\r\n");
        printf("  -speed <mhz>           Run the emulated CPU at approximately <mhz> MHz. (Default is as fast as possible)\r\n");
        printf("                         There is currently no clock ticks counted per instruction, so the emulator is just going\r\n");
        printf("                         to estimate how many instructions would come out to approximately the desired speed.\r\n");
        printf("                         There will be more accurate speed-throttling at some point in the future.\r\n\r\n");
        printf("Miscellaneous options:\r\n");
        printf("  -debug <level>         <level> can be: NONE, ERROR, INFO, DETAIL. (Default is INFO)\r\n");
        printf("  -mips                  Display live MIPS being emulated.\r\n");
        printf("  -h                     Show this help screen.\r\n");
}

int args_parse(MACHINE_t* machine, int argc, char* argv[]) {
        int i;
/*
        if (argc < 2) {
                printf("Specify command line parameters. Use -h for help.\r\n");
                return -1;
        }
*/
        for (i = 1; i < argc; i++) {
                if (args_isMatch(argv[i], "-h")) {
                        args_showHelp();
                        return -1;
                }
                else if (args_isMatch(argv[i], "-machine")) {
                        if ((i + 1) == argc) {
                                printf("Parameter required for -machine. Use -h for help.\r\n");
                                return -1;
                        }
                        i++;
                        if (args_isMatch(argv[i], "list")) {
                                machine_list();
                                return -1;
                        }
                        usemachine = argv[i];
                }
                else if (args_isMatch(argv[i], "-speed")) {
                        if ((i + 1) == argc) {
                                printf("Parameter required for -speed. Use -h for help.\r\n");
                                return -1;
                        }
                        speedarg = atof(argv[++i]);
                }
                else if (args_isMatch(argv[i], "-debug")) {
                        if ((i + 1) == argc) {
                                printf("Parameter required for -debug. Use -h for help.\r\n");
                                return -1;
                        }
                        if (args_isMatch(argv[i + 1], "none")) debug_setLevel(DEBUG_NONE);
                        else if (args_isMatch(argv[i + 1], "error")) debug_setLevel(DEBUG_ERROR);
                        else if (args_isMatch(argv[i + 1], "info")) debug_setLevel(DEBUG_INFO);
                        else if (args_isMatch(argv[i + 1], "detail")) debug_setLevel(DEBUG_DETAIL);
                        else {
                                printf("%s is an invalid debug option\r\n", argv[i + 1]);
                                return -1;
                        }
                        i++;
                }
                else if (args_isMatch(argv[i], "-mips")) {
                        showMIPS = 1;
                }
                else {
                        printf("%s is not a valid parameter. Use -h for help.\r\n", argv[i]);
                        return -1;
                }
        }

        return 0;
}
