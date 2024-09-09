/*
 * i2cThread.h
 *
 *  Created on: Mar 22, 2019
 *      Author: dig
 */

#ifndef I2CTHREAD_H_
#define I2CTHREAD_H_
#include <stdbool.h>
#include <stdint.h>

extern volatile uint32_t bellButtons;  // bit 0 - 29 bells no 2 - 60
extern bool LEDD4;						// LEDs written  to i2C
extern bool LEDD5;

void* i2cThread(void* args);

#endif /* I2CTHREAD_H_ */
