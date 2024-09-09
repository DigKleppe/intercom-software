/*
 * timerThread.h
 *
 *  Created on: May 5, 2019
 *      Author: dig
 */

#ifndef TIMERTHREAD_H_
#define TIMERTHREAD_H_

void* timerThread(void* args);
extern uint32_t upTime;
extern uint32_t activeTimer[NR_STATIONS];
extern uint32_t timeoutTimer[NR_STATIONS];
extern uint32_t commandTimer[NR_STATIONS];
extern uint32_t openDoorTimer;


#endif /* TIMERTHREAD_H_ */
