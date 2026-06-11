#include "memory.h"
#include "utils.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_i2c.h"

#include <string.h>

#define MEMID 0xA0



//#define PINSEL1 *(unsigned long *) 0x4002C004

void Memory_Init() {
	PINSEL_CFG_Type pinsel1;
	pinsel1.Portnum = 0;
	pinsel1.Pinnum  = 27;
	pinsel1.Funcnum = 1;
	pinsel1.Pinmode = PINSEL_PINMODE_PULLUP;
	pinsel1.OpenDrain = PINSEL_PINMODE_NORMAL;
	
	PINSEL_ConfigPin(&pinsel1);
	pinsel1.Pinnum = 28;
	PINSEL_ConfigPin(&pinsel1);
	
	I2C_Init(LPC_I2C0, 1000000);
	I2C_Cmd(LPC_I2C0, ENABLE);
}

/**
 * Write a table of size max 128. Will clamp the size of size > 128
 */ 
void Memory_Write(int addr, void* data, size_t size) {
	I2C_M_SETUP_Type setup;
	uint8_t tabval[129];
	
	tabval[0] = addr & 0xFF;
	size = min(size, 128);
  memcpy(&tabval[1], data, size);
	
	setup.sl_addr7bit = (MEMID |((addr & 0x700) >> 7)) >> 1;
	setup.tx_data     = tabval;
	setup.tx_length   = size + 1;
	setup.tx_count    = 0;
	
	setup.rx_data     = NULL;
	setup.rx_length   = 0;
	setup.rx_count    = 0;
	
	setup.retransmissions_count = 1;
	setup.retransmissions_max = 1;
	
	I2C_MasterTransferData(LPC_I2C0, &setup, I2C_TRANSFER_POLLING);
}

void Memory_Read(int addr, void* data, size_t size) {
	I2C_M_SETUP_Type setup;
	
	uint8_t tabval[1];
	tabval[0] = addr & 0xFF;
	
	setup.sl_addr7bit = (MEMID |((addr & 0x700) >> 7)) >> 1;
	setup.tx_data     = tabval;
	setup.tx_length   = 1;
	setup.tx_count    = 0;
	
	setup.rx_data     = data;
	setup.rx_length   = size;
	setup.rx_count    = 0;
	
	setup.retransmissions_count = 1;
	setup.retransmissions_max = 1;
	
	I2C_MasterTransferData(LPC_I2C0, &setup, I2C_TRANSFER_POLLING);
}