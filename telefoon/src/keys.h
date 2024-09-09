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

#define KEY_P1 			(1)
#define KEY_P2			(2)
#define KEY_P3 			(4)
#define KEY_HANDSET		(8)


#define ALLKEYS (	KEY_P1 |  KEY_P2 |  KEY_P3 |  KEY_HANDSET)

void* keysThread(void* args);



extern uint32_t keysRT;
extern uint32_t keyRepeats;
extern int16_t keysThreadCntr;

#endif /* KEYS_H_ */
