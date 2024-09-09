/*
 * mcp23017.c
 *
 *  Created on: Mar 17, 2019
 *      Author: dig
 * mcp23017.c : Interface with MCP23017 I/O Extender Chips
 *
 *********************************************************************/

#include "mcp23017.h"
#include <stdio.h>
#include <stdint.h>


#define IODIR	0
#define IPOL	1
#define GPINTEN	2
#define DEFVAL	3
#define INTCON	4
#define IOCON	5
#define GPPU	6
#define INTF	7
#define INTCAP	8
#define GPIO	9
#define OLAT	10

#define MCP_REGISTER(r,g) (((r)<<1)|(g)) /* For I2C routines */

//#include "sysgpio.c"
//
///*
// * Signal handler to quit the program :
// */
//
//static void
//sigint_handler(int signo) {
//	is_signaled = 1;		/* Signal to exit program */
//}

/*
 * Write to MCP23017 A or B register set:
 * return 0 OK
 */
int mcp23017_write(int gpio_addr, int reg,int AB,int value) {
	unsigned reg_addr = MCP_REGISTER(reg,AB);
	int rc;
	rc = i2c_write8(gpio_addr,reg_addr,value);
	return rc;
}

/*
 * Write value to both MCP23017 register sets :
 */
void mcp23017_write_both(int gpio_addr, int reg,int value) {
	mcp23017_write(gpio_addr, reg,GPIOA,value);	/* Set A */
	mcp23017_write(gpio_addr, reg,GPIOB,value);	/* Set B */
}

/*
 * Read the MCP23017 input pins 16-bits
 */

unsigned int  mcp23017_inputs(int gpio_addr) {
	unsigned int reg_addr = MCP_REGISTER(GPIO,GPIOA);
	uint16_t val = i2c_read16(gpio_addr,reg_addr);
	uint16_t swap = val << 8;
	swap += val >> 8;
	return swap;
}

/*
 * Write 16-bits to outputs :
 */
int mcp23017_outputs(int gpio_addr, int value) {
	unsigned reg_addr = MCP_REGISTER(GPIO,GPIOA);
	return i2c_write16(gpio_addr,reg_addr,value);
}

//
//unsigned mcp23017_captured(void) {
//	unsigned reg_addr = MCP_REGISTER(INTCAP,GPIOA);
//	return i2c_read16(gpio_addr,reg_addr);
//}

/*
// * Read interrupting input flags (16-bits):
// */
//unsigned mcp23017_interrupts(void) {
//	unsigned reg_addr = MCP_REGISTER(INTF,GPIOA);
//
//	return i2c_read16(gpio_addr,reg_addr);
//}

/*
 * Configure the MCP23017 GPIO Extenders :
 */
int  mcp23017_init(void) {
	int res;
	res =  mcp23017_write(MOD1ADDR, IODIR,GPIOA, 0xFF); 	// portA all inputs
	if ( res != 0 ) { // error
		printf("Error mcp23017 MOD1\n");
		return MOD1ADDR;
	}

	mcp23017_write(MOD1ADDR,IODIR,GPIOB, 0xFF) ; 		// portB all inputs
	mcp23017_write_both(MOD1ADDR,IPOL,0xFF);   			// inverted polarity
	mcp23017_write_both(MOD1ADDR,GPINTEN,0x00);			// No interrupts enabled
	mcp23017_write_both(MOD1ADDR,GPPU,0);				// No pullups
	mcp23017_write_both(MOD1ADDR,IOCON,0b01000100);		// MIRROR=1,ODR=1
	mcp23017_write_both(MOD1ADDR,DEFVAL,0x00);			// Clear default value
	mcp23017_write_both(MOD1ADDR,OLAT,0x00);			// OLATx=0

	res =  mcp23017_write(MOD2ADDR, IODIR,GPIOA, 0xFF); // portA inputs,
	if ( res != 0 ) { // error
		printf("Error mcp23017 MOD2\n");
		return MOD2ADDR;
	}

	mcp23017_write(MOD2ADDR,IODIR,GPIOB, 0b00111111) ; 	// PORTB 0-5 inputs. 6- 7 LEDs D4 - D5
	mcp23017_write_both(MOD2ADDR,IPOL,0xFF);   			// inverted polarity
	mcp23017_write_both(MOD2ADDR,GPINTEN,0x00);			// No interrupts enabled
	mcp23017_write_both(MOD2ADDR,GPPU,0);				// No pullups
	mcp23017_write_both(MOD2ADDR,IOCON,0b01000100);		// MIRROR=1,ODR=1
	mcp23017_write_both(MOD2ADDR,DEFVAL,0x00);			// Clear default value
	mcp23017_write_both(MOD2ADDR,OLAT,0x00);			// OLATx=0

	return 0;
}


/*
 * Main program :
// */
//int
//main(int argc,char **argv) {
//	int int_flags, v;
//	int fd;
//
//	signal(SIGINT,sigint_handler);		/* Trap on SIGINT */
//
//	i2c_init(node);				/* Initialize for I2C */
//	mcp23017_init();			/* Configure MCP23017 @ 20 */
//
////	fd = gpio_open_edge(gpio_inta);		/* Configure INTA pin */
//
//	puts("Monitoring for MCP23017 input changes:\n");
//
//	do	{
//		gpio_poll(fd);			/* Pause until an interrupt */
//
//		int_flags = mcp23017_interrupts();
//		if ( int_flags ) {
//			v = mcp23017_captured();
//			printf("  Input change: flags %04X values %04X\n",
//				int_flags,v);
//			post_outputs();
//		}
//	} while ( !is_signaled );		/* Quit if ^C'd */
//
//	fputc('\n',stdout);
//
//	i2c_close();				/* Close I2C driver */
//	close(fd);				/* Close gpio17/value */
////	gpio_close(gpio_inta);			/* Unexport gpio17 */
//	return 0;
//}

/*********************************************************************
 * End mcp23017.c - Warren Gay
 * Mastering the Raspberry Pi - ISBN13: 978-1-484201-82-4
 * This source code is placed into the public domain.
 *********************************************************************/
