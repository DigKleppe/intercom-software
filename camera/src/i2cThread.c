/*
 * i2cThread.c
 *
 *  Created on: Mar 17, 2019
 *      Author: dig
 *
 *
 *      i2cdetect -y 0:
 *      MCP23017
 *      address 0x20 (addresslines low)
 *      connected to port 0
 */
#include "camera.h"

#include <unistd.h>
#include "mcp23017.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>


int testMezelf;
int testJohan;

static const char *node = "/dev/i2c-0";
#define DEBOUNCES 	20

volatile uint32_t bellButtons; 	// read from i2C
uint32_t bellSimButtons; 		// testkeys telephones

uint32_t i2CErrors;
bool LEDD4;						// LED 1 and 2 written  to i2C
bool LEDD5;						// LED 1 and 2 written  to i2C


void* i2cThread(void* args)
{
	int res;
	int32_t temp;
	uint32_t inputs;
	uint32_t lastInputs;
	uint32_t debounceCntr = DEBOUNCES;

	threadStatus_t  * pThreadStatus = args;
	int errorCntr = 0;

	res = i2c_init(node);
	res = mcp23017_init();

	do {

		if ( res != 0 ) {  // test without i2c
			while (1) {
				if ( testMezelf > 0 ) {
					bellButtons |= 1<<14;  // no 30
					testMezelf--;
				}
				else
					bellButtons = 0;
				usleep(10000);
			}
		}

		while (res == 0 ){  // run always if no errors
			usleep(10000);
			uint16_t val = LEDD4 * LEDD4MASK + LEDD5 * LEDD5MASK;
			val = ~val; // invert

			res |= mcp23017_outputs(LEDSADDR, val);
			temp = mcp23017_inputs(MOD1ADDR);
			if ( temp == -1)
				res = -1; // error
			else
				inputs = temp; // lower 16 buttons

			temp = mcp23017_inputs(MOD2ADDR); // higher 14 buttons
			if ( temp == -1)
				res = -1; // error
			else
				inputs += temp<<16;

			inputs &= 0B111111111111111111111111111111; // mask 30 inputs

			if (lastInputs == inputs){
				if ( debounceCntr > 0 ){
					debounceCntr--;
				}
				else
					bellButtons = inputs;
			}
			else
				debounceCntr = DEBOUNCES;

			if ( testMezelf > 0 ) {
				bellButtons |= 1<<14;  // no 30
				testMezelf--;
			}
			if ( testJohan > 0 ) {
			//	bellButtons |= 1<<(44/2-1);
				bellButtons |= 1<<(32/2-1);
				testJohan--;
			}


			lastInputs = inputs;
			if (res == 0)
				errorCntr = 0;
			else
				i2CErrors++;

		}
	} while (errorCntr < 5); // stop after 5 efforts

	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}
