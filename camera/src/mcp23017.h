/*
 * mcp23017.h
 *
 *  Created on: Mar 17, 2019
 *      Author: dig
 */
#ifndef MCP23017_H_
#define MCP23017_H_

#define MOD1ADDR 0x20  // in 1- 16  (= houseno 2 - 32 )
#define MOD2ADDR 0x24	// in 17-30 (= houseno 34 - 60, and LEDs )

#define GPIOA	0
#define GPIOB	1

#include "i2c_funcs.h"	/* I2C routines */

#define LEDSPORT GPIOB
#define LEDSADDR MOD2ADDR
#define LEDD4MASK (1<<6)
#define LEDD5MASK (1<<7)



//#include "sysgpio.c"
/*
 * Write to MCP23017 A or B register set:
 */
int mcp23017_write(int gpio_addr, int reg,int AB,int value);
/*
 * Write value to both MCP23017 register sets :
 */
void mcp23017_write_both(int gpio_addr,int reg,int value);
/*
 * Read the MCP23017 input pins
 */
unsigned mcp23017_inputs(int gpio_addr);
/*
 * Write 16-bits to outputs :
 */
int mcp23017_outputs(int gpio_addr,int value);/*

//unsigned mcp23017_captured(void);
///*
// * Read interrupting input flags (16-bits):
// */
//unsigned mcp23017_interrupts(void);
///*
// * Configure the MCP23017 GPIO Extender :
// */
int mcp23017_init(void);

#endif /* MCP23017_H_ */
