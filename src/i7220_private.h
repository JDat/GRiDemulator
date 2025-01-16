/*
  GRiD Compass emulator
  Copyright (C)2022 JDat
  https://github.com/JDat/GRiDemulator

  Based on MAMEdev project
  license:BSD-3-Clause
  copyright-holders:Sergey Svishchev
  https://github.com/mamedev/mame/blob/master/src/devices/machine/i7220.cpp

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

/*
	Bubble memory module emulator
*/

#ifndef _BUBBLE_PRIVATE_H_
#define _BUBBLE_PRIVATE_H_

#include <stdint.h>

enum regNames{
    regFIFO         = 0x00,
    regUtility      = 0x0a,
    regBlockLenLSB  = 0x0b,
    regBlockLenMSB  = 0x0c,
    regEnable       = 0x0d,
    regAddressLSB   = 0x0e,
    regAddressMSB   = 0x0f,
};



enum cmdNames{
    cmdWrBootLoopRegMasked = 0,
    cmdInitialize = 1,
    cmdReadBubbleData = 2,
    cmdWriteBubbleData = 3,
    cmdReadSeek = 4,
    cmdRdBootLoopReg = 5,
    cmdWrBootLoopReg = 6,
    cmdWrBootLoop = 7,
    cmdRdFsaStatus= 8,
    cmdAbort = 9,
    cmdWriteSeek = 10,
    cmdRdBootLoop = 11,
    cmdRdCorrectedData = 12,
    cmdResetFifo = 13,
    cmdMbmPurge = 14,
    cmdSoftReset = 15,
};

enum statusBits {
    BIT_FIFO_READY          = 0,
    BIT_PARITY_ERROR        = 1,
    BIT_UNCORRECTABLE_ERROR = 2,
    BIT_CORRECTABLE_ERROR   = 3,
    BIT_TIMING_ERROR        = 4,
    BIT_OP_FAIL             = 5,
    BIT_OP_COMPLETE         = 6,
    BIT_BUSY                = 7,
};

enum enableBits {
    BIT_IRQ_NORMAL          = 0,
    BIT_IRQ_ERROR           = 1,
    BIT_DMA_ENABLED         = 2,
    BIT_MAX_FSA_XFER_RATE   = 3,
    BIT_WRITE_BOOTLOOP_ENA  = 4,
    BIT_RCD_ENA             = 5,
    BIT_ICD_ENA             = 6,
    BIT_PARITY_INTERRUPT    = 7,
};

enum cmdStates{
    PHASE_IDLE, PHASE_CMD, PHASE_EXEC, PHASE_RESULT
};


#endif
