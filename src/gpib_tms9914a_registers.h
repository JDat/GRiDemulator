#ifndef _GPIB_TMS9914A_REGISTERS_H_
#define _GPIB_TMS9914A_REGISTERS_H_

#include <stdint.h>

enum {
	AUXCMD_SWRST = 0x00,
	AUXCMD_DACR = 0x01,
	AUXCMD_RHDF = 0x02,
	AUXCMD_HDFA = 0x03,
	AUXCMD_HDFE = 0x04,
	AUXCMD_NBAF = 0x05,
	AUXCMD_FGET = 0x06,
	AUXCMD_RTL = 0x07,
	AUXCMD_FEOI = 0x08,
	AUXCMD_LON = 0x09,
	AUXCMD_TON = 0x0a,
	AUXCMD_GTS = 0x0b,
	AUXCMD_TCA = 0x0c,
	AUXCMD_TCS = 0x0d,
	AUXCMD_RPP = 0x0e,
	AUXCMD_SIC = 0x0f,
	AUXCMD_SRE = 0x10,
	AUXCMD_RQC = 0x11,
	AUXCMD_RLC = 0x12,
	AUXCMD_DAI = 0x13,
	AUXCMD_PTS = 0x14,
	AUXCMD_STDL = 0x15,
	AUXCMD_SHDW = 0x16,
	AUXCMD_VSTDL = 0x17,
	AUXCMD_RSV2 = 0x18
};

struct _auxCmd {
  uint8_t cmd;
  char name[32];
};

struct _auxCmd auxCmd[] = {
  {AUXCMD_SWRST,  "Software reset"},
  {AUXCMD_DACR,   "Release DAC holdoff"},
  {AUXCMD_RHDF,   "Release RFD holdoff"},
  {AUXCMD_HDFA,   "Holdoff on all data"},
  {AUXCMD_HDFE,   "Holdoff on EOI only"},
  {AUXCMD_NBAF,   "New byte available false"},
  {AUXCMD_FGET,   "Force group execute trigger"},
  {AUXCMD_RTL,    "Return to local"},
  {AUXCMD_FEOI,   "Send EOI with next byte"},
  {AUXCMD_LON,    "Listen only"},
  {AUXCMD_TON,    "Talk only"},
  {AUXCMD_GTS,    "Go to standby"},
  {AUXCMD_TCA,    "Take control asynchronously"},
  {AUXCMD_TCS,    "Take control synchronously"},
  {AUXCMD_RPP,    "Request parallel pool"},
  {AUXCMD_SIC,    "Send interface clear"},
  {AUXCMD_SRE,    "Send remote enable"},
  {AUXCMD_RQC,    "Request control"},
  {AUXCMD_RLC,    "Release control"},
  {AUXCMD_DAI,    "Disable all interruptrs"},
  {AUXCMD_PTS,    "Pass through next secondary"},
  {AUXCMD_STDL,   "Short T1 setting time"},
  {AUXCMD_SHDW,   "Shadow handshake"},
  {AUXCMD_VSTDL,  "Very short T1 delay"},
  {AUXCMD_RSV2,   "Request Service Bit 2"}  
};


#endif
