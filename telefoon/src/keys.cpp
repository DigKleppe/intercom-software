/*
 * keys.c
 *
 *  Created on: Apr 8, 2019
 *      Author: dig
 */
#include "telefoon.h"
#include "io.h"
#include "keys.h"

#include <unistd.h>

#define KEYSSAMPLEINTERVAL	5 //ms
#define DEBOUNCES			20/KEYSSAMPLEINTERVAL
#define KEY_REPEAT_TIME		400/KEYSSAMPLEINTERVAL
#define KEYACCEL 			10
#define MAXREPEAT			200/KEYSSAMPLEINTERVAL

uint32_t keyRepeats = 0;	//bit set to repeat
uint32_t keysRT = 0;
int16_t keysThreadCntr;

static uint32_t keyReadBits = 0;

/*********************************************************************
 *
 *   check if key is pressed
 *   input:   reqkey ( one or more bits)
 *   return:  reqkey bits if in ( or repeating )
 *
 *********************************************************************/

uint32_t key ( uint32_t reqKey ) {
	uint32_t result = 0;
	keyReadBits &= keysRT;   // clear read flags not pressed keys
	result = reqKey & keysRT & ~keyReadBits ; // bit(s) set if key in and not yet read
	keyReadBits |= result;  //  set bits read , if correspondig repeat bits are set bits are cleared if key stays in, after repeat time
	return( result);
}

// takes care of repeating keys

static void keyProcess ( void ){
	static uint32_t keyrepeatTimer = 0xFFFF;
	static uint32_t repeatTime = KEY_REPEAT_TIME;

	keyReadBits &= keysRT;            // clear read flags not pressed keys

	if( keysRT & keyRepeats ) {           // repeating key in?
		if( keyrepeatTimer == 0xFFFF ) { // first time repeat?
			keyrepeatTimer = KEY_REPEAT_TIME;
			repeatTime = KEY_REPEAT_TIME;
		}
		else {
			keyrepeatTimer--;
			if( !keyrepeatTimer ) {
				keyrepeatTimer = repeatTime;
				if (repeatTime > MAXREPEAT )
					repeatTime  -= KEYACCEL;
				keyReadBits &= ~keyRepeats; // clr keysRead flags of repating keys
			}
		}
	}
	else
		keyrepeatTimer = 0xFFFF;
}


#define testInterval  (20 * 1000 / KEYSSAMPLEINTERVAL)

void* keysThread(void* args)
{
	int res;
	int32_t temp;
	uint32_t inputs;
	uint32_t lastInputs;
	uint32_t debounceCntr = DEBOUNCES;
#ifdef SIM
	int32_t testTimer = 0; //10 * 1000 / KEYSSAMPLEINTERVAL;
	int presc = 1000/KEYSSAMPLEINTERVAL;
#endif

	while (1)  {
		usleep(1000 * KEYSSAMPLEINTERVAL);
		keysThreadCntr++;
#ifndef SIM
		inputs = getSwitches();
		if (lastInputs == inputs){
			if ( debounceCntr > 0 )
				debounceCntr--;
			else
				keysRT = inputs;
		}
		else
			debounceCntr = DEBOUNCES;
#else
		presc--;
		if (presc == 0){
			presc = 1000/KEYSSAMPLEINTERVAL;
			testTimer++;
			switch (testTimer) {
			case 10:
				keysRT = KEY_P3;
				break;
			case 12:
				keysRT = KEY_HANDSET; // horn off the hook
				break;
			case 17:
				keysRT = 0; // horn on the hook
				break;

			case 35:
				testTimer = 0;
				break;
			}
		}
#endif
		lastInputs = inputs;
		keyProcess();
	}
}



