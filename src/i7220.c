// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/**********************************************************************

    Intel 7220 Bubble Memory Controller emulation

    References:
    - http://www.decadecounter.com/vta/pdf/BPK72UM.pdf
    - http://www.decadecounter.com/vta/pdf/7220-1.pdf
    - http://www.decadecounter.com/vta/pdf/7242.pdf
    - http://www.decadecounter.com/vta/articleview.php?item=359

    Implemented at least partially
    - commands
        Read Bubble Data, Write Bubble Data
        Initialize, Reset FIFO, Abort, Software Reset
        Read FSA Status, MBM Purge

    Not implemented
    - interrupts
    - DMA
    - commands
        Read Seek, Write Seek, Read Corrected Data, all Bootloop-related
    - access to other than 2 FSA channels at once

**********************************************************************/

//#include "emu.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "machine.h"
#include "i7220.h"
#include "i8259.h"
//#include "ports.h"
#include "memory.h"
#include "debuglog.h"
#include "utility.h"
#include "timing.h"



#define baseAddress 0xDFE80
#define addressLen  0x4

void delay(int millis)
{
    // Converting time into milli_seconds
    int milli_seconds = millis;
 
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}

// device type definition
//DEFINE_DEVICE_TYPE(I7220, i7220_device, "i7220", "Intel 7220 BMC")

//-------------------------------------------------
//  i7220_device - constructor
//-------------------------------------------------

/*
i7220_device::i7220_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, I7220, tag, owner, clock)
	, device_image_interface(mconfig, *this)
	, intrq_cb(*this)
	, drq_cb(*this)
{
}
*/

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------
/*
void device_start()
{
	// resolve callbacks
	intrq_cb.resolve_safe();
	drq_cb.resolve_safe();

	m_bi.tm = timer_alloc(FUNC(i7220_device::general_continue), this);

	// register for state saving
	save_item(NAME(m_regs));
	save_item(NAME(m_rac));
	save_item(NAME(m_cmdr));
	save_item(NAME(m_str));
	save_item(NAME(m_blr));
	save_item(NAME(m_ar));
}
*/
//-------------------------------------------------
//  device_reset - device-specific startup
//-------------------------------------------------

I8259_t* irq8259;

int m_fifo_head, m_fifo_tail;

FILE *bubbleFile;

uint8_t bubble_init(I8259_t* i8259) {
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] Initializing bubble memory controller\r\n");
#endif
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)bubble_read, (void*)bubble_write, NULL);
        device_reset();
        
        irq8259 = i8259;
        if (bubbleFile != NULL) {
                fclose(bubbleFile);
        }
        bubbleFile = fopen("ROMS/bubble.img", "rwb");
        if (bubbleFile == NULL) {
#ifdef DEBUG_BUBBLEMEM
                debug_log(DEBUG_INFO, "[i7220] Error openimg bubble image\r\n");
#endif
                return -1;
        }
        timing_addTimer(bubble_timer, (void*)(NULL), 50000, TIMING_ENABLED); //79545.47
        set_data_size(3);       // 3 bubble memory modules
        return 0;
}

void set_data_size(int data_size) { m_data_size = data_size; }

void device_reset()
{
	m_main_phase = PHASE_IDLE;
	m_bi.main_state = IDLE;
	m_bi.sub_state = IDLE;

	set_drq(false);
	set_irq(false);
	fifo_clear();

	m_rac = m_cmdr = m_str = m_blr = m_ar = 0;
	memset(&m_regs, 0, sizeof(m_regs));
}

/*
uint8_t call_load()
{
	if (length() != (m_data_size * I7110_MBM_SIZE))
	{
		return -1;
	}
	return 0;
}
*/

void set_drq(bool state)
{
        //I8259_t* i8259;
        //i8259 = i8253cb->i8259;
	if (state != m_drq)
	{
		m_drq = state;
		//drq_cb(m_drq);
#ifdef DEBUG_BUBBLEMEM
                debug_log(DEBUG_INFO, "[i7220] DMA request state:%d\n", state);
#endif
                //set_irq(m_drq);
                //dmaBubbleRequest();
                if (m_drq) {
                        i8259_doirq(irq8259, 5);
                        //set_irq(true);
                }
	}
}

void set_irq(bool state)
{
	if (state != m_irq)
	{
		m_irq = state;
		//intrq_cb(m_irq);
#ifdef DEBUG_BUBBLEMEM
                debug_log(DEBUG_INFO, "[7220] IRQ request %d\n", state);
#endif
                if (m_irq) {
                        i8259_doirq(irq8259, 5);
                }
	}
}

void update_drq()
{
        
	switch (m_bi.main_state)
	{
	case READ_DATA:
		set_drq(m_fifo_size < 22 ? false : true);
		break;

	case WRITE_DATA:
		set_drq(m_fifo_size < (40 - 22) ? true : false);
		break;
	}
        
}


void fifo_clear()
{
	//m_fifo.clear();
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] fifo clear\n");
#endif
	m_fifo_size = 0;
	set_drq(false);
        m_fifo_head = m_fifo_tail = 0;
}

uint8_t fifo_pop()
{
        if (m_fifo_head == m_fifo_tail) return 0;
        
        uint8_t val;
        val = m_fifo[m_fifo_tail];
        m_fifo_tail = (m_fifo_tail + 1) % BUBBLEFIFOSIZE;
        if (m_main_phase == PHASE_EXEC) {
                update_drq();
        }
        m_fifo_size --;
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] fifo pop: %02X\n", val);
#endif
        return val;
/*
	uint8_t val;

	if (m_fifo.empty())
	{
		return m_fifo.dequeue();
	}
	else
	{
		val = m_fifo.dequeue();
		m_fifo_size--;
		if (m_main_phase == PHASE_EXEC)
		{
			update_drq();
		}
		return val;
	}
*/
}


void fifo_push(uint8_t val)
{
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] fifo push: %02X\n", val);
#endif
        uint8_t i;
        i = (m_fifo_head + 1) % BUBBLEFIFOSIZE;
        if (i != m_fifo_tail) {
                m_fifo[m_fifo_head] = val;
                m_fifo_head = i;
                m_fifo_size ++;
                if (m_main_phase == PHASE_EXEC)
                {
                        update_drq();
                }
        }

/*
	if (m_fifo.full())
	{
		return;
	}
	else
	{
		m_fifo.enqueue(val);
		m_fifo_size++;
		if (m_main_phase == PHASE_EXEC)
		{
			update_drq();
		}
	}
*/
}


void update_regs()
{
	m_blr = (m_regs[R_BLR_M] << 8) + m_regs[R_BLR_L];
	m_ar = (m_regs[R_AR_M] << 8) + m_regs[R_AR_L];
	m_blr_count = m_blr & 0x7ff;
	m_blr_nfc = (m_blr >> 12) ? ((m_blr >> 12) << 1) : 1;
	m_ar_addr = m_ar & 0x7ff;
	m_ar_mbm = (m_ar >> 11) & 15;
}


void start_command(int cmd)
{
	m_main_phase = PHASE_EXEC;
	m_str &= ~SR_CLEAR;

	switch (cmd)
	{
	// determines how many FSAs are present, reads and decodes bootloop from each MBM and stores result in FSA bootloop regs.
	// all parametric registers must be properly set up before issuing Initialize command.
	// NFC bits in BLR MSB must be set to 0001 before issuing this command.
	// MBM GROUP SELECT bits in the AR must select the last MBM in the system.
	case C_INIT:
		//debug_log(DEBUG_INFO, "BMC INIT: BLR %04x (NFC %d pages %d) AR %04x (MBM %d addr %03x) ER %02d\n",
		//	m_blr, m_blr_nfc, m_blr_count, m_ar, m_ar_mbm, m_ar_addr, m_regs[R_ER]);
		if (m_blr_nfc != 2)
		{
			command_fail_start();
		}
		else
		{
			init_start();
		}
		break;

	case C_READ_FSA_STATUS:
		read_fsa_start();
		break;

	// all parametric registers must be properly set up before issuing Read Bubble Data command
	case C_READ:
		//debug_log(DEBUG_INFO, "BMC RBD: BLR %04x (NFC %d pages %d) AR %04x (MBM %d addr %03x) ER %02d\n",
		//	m_blr, m_blr_nfc, m_blr_count, m_ar, m_ar_mbm, m_ar_addr, m_regs[R_ER]);
		if (m_ar_mbm >= m_data_size || m_blr_nfc != 2)
		{
			command_fail_start();
		}
		else
		{
			read_data_start();
		}
		break;

	case C_WRITE:
		//debug_log(DEBUG_INFO, "BMC WBD: BLR %04x (NFC %d pages %d) AR %04x (MBM %d addr %03x) ER %02d\n",
		//	m_blr, m_blr_nfc, m_blr_count, m_ar, m_ar_mbm, m_ar_addr, m_regs[R_ER]);
		if (m_ar_mbm >= m_data_size || m_blr_nfc != 2)
		{
			command_fail_start();
		}
		else
		{
			write_data_start();
		}
		break;

	// XXX clears all BMC registers (except BLR &c), counters and the MBM address RAM.  used by Initialize as subroutine.
	case C_MBM_PURGE:
		m_regs[R_AR_L] = 0;
		m_regs[R_AR_M] &= ~7;
		update_regs();
		m_main_phase = PHASE_RESULT;
		m_str |= SR_DONE;
		break;

	// controlled termination of currently executing command.  command accepted in BUSY state.
	// if not BUSY, clears FIFO.
	case C_ABORT:
		if (m_main_phase == PHASE_IDLE)
		{
			fifo_clear();
		}
		else
		{
			m_main_phase = PHASE_RESULT;
			m_bi.main_state = m_bi.sub_state = IDLE;
		}
		m_str |= SR_DONE;
		break;

	case C_RESET_FIFO:
		fifo_clear();
		m_main_phase = PHASE_RESULT;
		m_str |= SR_DONE;
		break;

	// clears BMC FIFO and all registers except those containing init parameters.  sends Reset to every FSA.
	case C_RESET:
		m_regs[R_UR] = 0;
		fifo_clear();
		m_main_phase = PHASE_RESULT;
		m_str |= SR_DONE;
		break;

	default:
		command_fail_start();
		break;
	}
}

void  bubble_timer()
{
        //debug_log(DEBUG_INFO, "[i7220] timer call\n");
	switch (m_bi.main_state)
	{
	case IDLE:
		break;

	case INIT:
		init_continue();
		break;

	case FAIL:
		command_fail_continue();
		break;

	case READ_FSA:
		read_fsa_continue();
		break;

	case READ_DATA:
		read_data_continue();
		break;

	case WRITE_DATA:
		write_data_continue();
		break;

	default:
		//debug_log(DEBUG_INFO, "BMC general_continue on unknown main-state %d\n", m_bi.main_state);
		break;
	}
}

//void delay_cycles(emu_timer *tm, int cycles)
//{
//	tm->adjust(attotime::from_double(double(cycles) / clock()));
//}

void delay_cycles(int cycles)
{
	//tm->adjust(attotime::from_double(double(cycles) / clock()));
        delay(1);
}


void command_end(bool success)
{
	//debug_log(DEBUG_INFO, "command done (%s) - %02x\n", success ? "success" : "fail", m_str);
	m_main_phase = PHASE_RESULT;
	m_bi.main_state = m_bi.sub_state = IDLE;
	if (success)
	{
		m_str |= SR_DONE;
	}
	else
	{
		m_str |= SR_FAIL | SR_TE;
	}
}


void command_fail_start()
{
	m_bi.main_state = FAIL;
	m_bi.sub_state = INITIALIZE;

	command_fail_continue();
}

void command_fail_continue()
{
	switch (m_bi.sub_state)
	{
	case INITIALIZE:
		m_bi.sub_state = COMMAND_DONE;
		//delay_cycles(m_bi.tm, 1200); // XXX
                delay_cycles(1200); // XXX
		return;

	case COMMAND_DONE:
		command_end(false);
		return;

	default:
		//debug_log(DEBUG_INFO, "BMC fail unknown sub-state %d\n", m_bi.sub_state);
		return;
	}
}

void init_start()
{
	m_bi.main_state = INIT;
	m_bi.sub_state = INITIALIZE;

	init_continue();
}

void init_continue()
{
	for (;;)
	{
		switch (m_bi.sub_state)
		{
		case INITIALIZE:
			m_bi.sub_state = WAIT_FSA_REPLY;
			//delay_cycles(m_bi.tm, m_data_size * 60); // p. 4-16 of BPK72UM
                        delay_cycles(m_data_size * 60); // p. 4-16 of BPK72UM
			return;

		case WAIT_FSA_REPLY:
			m_bi.sub_state = COMMAND_DONE;
			break;

		case COMMAND_DONE:
			command_end(true);
			return;

		default:
			//debug_log(DEBUG_INFO, "BMC init unknown sub-state %d\n", m_bi.sub_state);
			return;
		}
	}
}

void read_fsa_start()
{
	m_bi.main_state = READ_FSA;
	m_bi.sub_state = INITIALIZE;

	read_fsa_continue();
}

void read_fsa_continue()
{
	for (;;)
	{
		switch (m_bi.sub_state)
		{
		case INITIALIZE:
			m_bi.sub_state = WAIT_FSA_REPLY;
			//delay_cycles(m_bi.tm, m_data_size * 60); // p. 4-16 of BPK72UM
                        delay_cycles(m_data_size * 60); // p. 4-16 of BPK72UM
			return;

		case WAIT_FSA_REPLY:
			m_bi.sub_state = COMMAND_DONE;
			break;

		case COMMAND_DONE:
			// each bubble memory module has two FSA channels
			for (int a = 0; a < m_data_size; a++)
			{
				fifo_push(0x28); // FIFOMT | ECF/F
				fifo_push(0x28);
			}
			command_end(true);
			return;

		default:
			debug_log(DEBUG_INFO, "BMC read fsa unknown sub-state %d\n", m_bi.sub_state);
			return;
		}
	}
}

void read_data_start()
{
	m_bi.main_state = READ_DATA;
	m_bi.sub_state = INITIALIZE;

	read_data_continue();
}

void read_data_continue()
{
	for (;;)
	{
                size_t ret;
		switch (m_bi.sub_state)
		{
		case INITIALIZE:
			m_bi.sub_state = SECTOR_READ;
			m_bi.counter = 0; // 256-bit pages
			m_bi.limit = m_blr_count * m_blr_nfc;
			fseek(bubbleFile, (m_ar_addr * 32 * m_blr_nfc) + (m_ar_mbm * I7110_MBM_SIZE) + (m_bi.counter * 32), SEEK_SET);
                        //debug_log(DEBUG_INFO, "[i7220] [file seek] ftell: %d\n", ftell(bubbleFile) );
			break;

		case SECTOR_READ:
			//debug_log(DEBUG_INFO, "[i7220] [file access] read data. ftell: %d\n", ftell(bubbleFile) );
                        ret = fread(m_buf, 32, 1, bubbleFile);
			m_bi.sub_state = WAIT_FSA_REPLY;
			//delay_cycles(m_bi.tm, 270 * 20); // p. 4-14 of BPK72UM
                        delay_cycles(270 * 20); // p. 4-14 of BPK72UM
			break;

		case WAIT_FSA_REPLY:
			//debug_log(DEBUG_INFO, "[i7220] [file access] read data: ct %02d limit %02d\n", m_bi.counter, m_bi.limit);
			if (m_bi.counter < m_bi.limit)
			{
				for (int a = 0; a < 32; a++)
					fifo_push(m_buf[a]);
				m_bi.sub_state = SECTOR_READ;
				m_bi.counter++;
				//delay_cycles(m_bi.tm, 270 * 20); // p. 4-14 of BPK72UM
                                delay_cycles(270 * 20); // p. 4-14 of BPK72UM
                                
				return;
			}
			m_bi.sub_state = COMMAND_DONE;
			break;

		case COMMAND_DONE:
			command_end(true);
                        //debug_log(DEBUG_INFO, "[i7220] [file access] COMMAND_DONE\n");
                        set_irq(true);
                        delay_cycles(270 * 20); // p. 4-14 of BPK72UM
                        set_irq(false);
			return;

		default:
			//debug_log(DEBUG_INFO, "BMC read data unknown sub-state %d\n", m_bi.sub_state);
			return;
		}
	}
}

void write_data_start()
{
	m_bi.main_state = WRITE_DATA;
	m_bi.sub_state = INITIALIZE;

	write_data_continue();
}

void write_data_continue()
{
	for (;;)
	{
		switch (m_bi.sub_state)
		{
		case INITIALIZE:
			m_bi.sub_state = WAIT_FIFO;
			m_bi.counter = 0;
			m_bi.limit = m_blr_count * m_blr_nfc * 32;
			//delay_cycles(m_bi.tm, 270 * 20); // p. 4-14 of BPK72UM
                        delay_cycles(270 * 20); // p. 4-14 of BPK72UM
			return;

		case WAIT_FIFO:
			//debug_log(DEBUG_INFO, "[i7220] [file access] write data: fifo %02d ct %02d limit %02d\n", m_fifo_size, m_bi.counter, m_bi.limit);
			if (m_fifo_size >= 32)
			{
				for (int a = 0; a < 32; a++)
					m_buf[a] = fifo_pop();
				fseek(bubbleFile, (m_ar_addr * 32 * m_blr_nfc) + (m_ar_mbm * I7110_MBM_SIZE) + m_bi.counter, SEEK_SET);
				fwrite(m_buf, 32, 1, bubbleFile);
				m_bi.counter += 32;
			}
			if (m_bi.counter < m_bi.limit)
			{
				//delay_cycles(m_bi.tm, 270 * 20); // p. 4-14 of BPK72UM
                                delay_cycles(270 * 20); // p. 4-14 of BPK72UM
				return;
			}
			m_bi.sub_state = COMMAND_DONE;
			break;

		case COMMAND_DONE:
			command_end(true);
			return;

		default:
			//debug_log(DEBUG_INFO, "BMC write data unknown sub-state %d\n", m_bi.sub_state);
			return;
		}
	}
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

uint8_t bubble_read(void* dummy, uint32_t offset)
{
	uint8_t data = 0;
        
        //bubble_timer();

        offset = offset - baseAddress;
        offset = offset >> 1;

        //debug_log(DEBUG_INFO, "[i7220] Read addr: %05X\n", offset);

	//switch (offset & 1)
        switch (offset)
	{
	case 0:
		if (m_rac)
		{
			data = m_regs[m_rac];
			//debug_log(DEBUG_INFO, "BMC R reg @ %02x == %02x\n", m_rac, data);
			m_rac++;
			m_rac &= 15;
		}
		else
		{
			data = fifo_pop();
			//debug_log(DEBUG_INFO, "BMC R fifo == %02x\n", data);
		}
		break;

	case 1:
		data = m_str;
		if (m_main_phase == PHASE_EXEC)
		{
			data |= SR_BUSY;
			switch (m_bi.main_state)
			{
			case READ_DATA:
                                if (m_fifo_head != m_fifo_tail)
				//if (!m_fifo.empty()) // XXX
				{
					data |= SR_FIFO;
				}
				break;

			case WRITE_DATA:
                                if ((m_fifo_head + 1) % BUBBLEFIFOSIZE != m_fifo_tail)
				//if (!m_fifo.full()) // XXX
				{
					data |= SR_FIFO;
				}
				break;
			}
		}
		else
		{
                        if (m_fifo_head != m_fifo_tail)
			//if (!m_fifo.empty())
			{
				data |= SR_FIFO;
			}
		}
		//debug_log(DEBUG_INFO, "BMC R status == %02x (phase %d state %d:%d fifo %d drq %d)\n",
		//	data, m_main_phase, m_bi.main_state, m_bi.sub_state, m_fifo_size, m_drq);
		if (m_main_phase == PHASE_RESULT)
		{
			m_main_phase = PHASE_IDLE;
		}
		break;
	}

	//debug_log(DEBUG_INFO, "BMC R @ %d == %02x\n", offset, data);

	return data;
}

//-------------------------------------------------
//  write -
//-------------------------------------------------

void bubble_write(void* dummy, uint32_t offset, uint8_t data)
{
	static const char *commands[] = {
		"Write Bootloop Register Masked",
		"Initialize",
		"Read Bubble Data",
		"Write Bubble Data",
		"Read Seek",
		"Read Bootloop Register",
		"Write Bootloop Register",
		"Write Bootloop",
		"Read FSA Status",
		"Abort",
		"Write Seek",
		"Read Bootloop",
		"Read Corrected Data",
		"Reset FIFO",
		"MBM Purge",
		"Software Reset"
	};

        //bubble_timer();

        offset = offset - baseAddress;
        offset = offset >> 1;

        //debug_log(DEBUG_INFO, "[i7220] Write addr: %05X\tdata: %02X\n", offset, data);
        
	//debug_log(DEBUG_INFO, "BMC W @ %d <- %02x\n", offset, data);

	//switch (offset & 1)
        switch (offset)
	{
	case 0:
		if (m_rac)
		{
			//debug_log(DEBUG_INFO, "BMC W reg @ %02x <- %02x\n", m_rac, data);
			m_regs[m_rac] = data;
			update_regs();
			m_rac++;
			m_rac &= 15;
		}
		else
		{
			//debug_log(DEBUG_INFO, "BMC W fifo <- %02x\n", data);
			fifo_push(data);
		}
		break;

	case 1:
		if (bitRead(data, 5))
		{
			//intrq_cb(CLEAR_LINE);
		}
		else if (bitRead(data, 4))
		{
			m_cmdr = data & 15;
			if (m_main_phase == PHASE_IDLE)
			{
				//debug_log(DEBUG_INFO, "BMC command %02x '%s'\n", data, commands[m_cmdr]);
				m_main_phase = PHASE_CMD;
				start_command(m_cmdr);
			}
		}
		else
		{
			m_rac = data & 15;
		}
		break;
	}
	return;
}
