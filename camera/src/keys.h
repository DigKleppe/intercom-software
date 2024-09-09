/*
 * keys.h
 *
 *  Created on: Apr 8, 2019
 *      Author: dig
 */

#ifndef KEYS_H_
#define KEYS_H_
#include <stdint.h>

uint32_t key ( uint32_t reqKey );

#define KEY_SW1			(1)
#define KEY_SW2			(2)
#define ALLKEYS 		(KEY_SW1 |  KEY_SW1)


// keys in telephone
#define KEY_P1 			(1)
#define KEY_P2			(2)
#define KEY_P3 			(4)
#define KEY_HANDSET		(8)



void* keysThread(void* args);


extern uint32_t keysRT;
extern uint32_t keyRepeats;

#endif /* KEYS_H_ */
