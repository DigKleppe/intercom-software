/*
 * timerThread.h
 *
 *  Created on: May 5, 2019
 *      Author: dig
 */

#ifndef TIMERTHREAD_H_
#define TIMERTHREAD_H_

void* timerThread(void* args);
extern volatile uint32_t upTime,updateTimeoutTimer,overallTimeoutTimer;
extern volatile uint32_t doorKeyTimer;
extern volatile uint32_t commandTimer;

#endif /* TIMERTHREAD_H_ */
