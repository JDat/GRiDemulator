#ifndef _TMS9914A_PRIVATE_H_
#define _TMS9914A_PRIVATE_H_

#include <stdint.h>

#define auxCmdMask 0b00011111
#define auxBitMask 0b10000000

// Auxiliary commands
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

// Auriliary command desctiption for debug
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

// Auxiliary bits
enum {
  flagSWRST   = 0,
  flagDACR    = 1,
  flagRHDF    = 2,
  flagHDFA    = 3,
  flagHDFE    = 4,
  flagNBAF    = 5,
  flagFGET    = 6,
  flagRTL     = 7,
  flagFEOI    = 8,
  flagLON     = 9,
  flagTON     = 10,
  flagGTS     = 11,
  flagTCA     = 12,
  flagTCS     = 13,
  flagRPP     = 14,
  flagSIC     = 15,
  flagSRE     = 16,
  flagRQC     = 17,
  flagRLCa    = 18,
  flagDAI     = 19,
  flagPTS     = 20,
  flagSTDL    = 21,
  flagSHDW    = 22,
  flagVSTDL   = 23,
  flagRSV2    = 24
};

// Registers (I/O access by CPU)
enum {
  // Read registers
  regIntStatus0       = 0,
  regIntStatus1       = 1,
  regAddressStatus    = 2,
  regBusStatus        = 3,
  regCmdPassThru      = 6,
  regDataIn           = 7,
  
  // Write registers
  regIntMask0         = 8,
  regIntMask1         = 9,
  regAuxCmd           = 11,
  regAddress          = 12,
  regSerialPool       = 13,
  regParallellPool    = 14,
  regDataOut          = 15
};

// Register flags
enum {
  flagINT0  = 0,
  flagINT1  = 1,
  flagBI    = 2,
  flagBO    = 3,
  flagEND   = 4,
  flagSPAS  = 5,
  flagRLC   = 6,
  flagMAC   = 7,
  
  flagGET   = 0,
  flagERR   = 1,
  flagUNC   = 2,
  flagAPT   = 3,
  flagDCAS  = 4,
  flagMA    = 5,
  flagSRQi  = 6,
  flagIFCi  = 7,
  
  flagREM   = 0,
  flagLLO   = 1,
  flagATNa  = 2,
  flagLPAS  = 3,
  flagTPAS  = 4,
  flagLADS  = 5,
  flagTADS  = 6,
  flagULPA  = 7,
  
  flagATNb  = 0,
  flagDAV   = 1,
  flagNDAC  = 2,
  flagNRFD  = 3,
  flagEOI   = 4,
  flagSRQb  = 5,
  flagIFCb  = 6,
  flagREN   = 7,
  
  flagCS    = 0,
  
  flagEDPA  = 0,
  flagDAL   = 1,
  flagDAT   = 2,
  
  flagRSVL  = 1
};

// Controller states
enum {
  FSM_C_CIDS,
  FSM_C_CADS,
  FSM_C_CACS,
  FSM_C_CSBS,
  FSM_C_CWAS,
  FSM_C_CSHS,
  FSM_C_CSWS,
  FSM_C_CAWS,
  FSM_C_CPWS
};
  
void update_int();
bool controller_reset();
bool get_ifcin();
void do_aux_cmd(unsigned cmd , bool set_bit);
void do_swrst();
void update_fsm();

#endif
