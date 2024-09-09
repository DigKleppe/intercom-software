/*
 * timerThread.c
 *
 *  Created on: May 5, 2019
 *      Author: dig
 */

#include "telefoon.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "io.h"

extern status_t status;

volatile uint32_t overallTimeoutTimer = NOCOMM_TIMEOUT;
volatile uint32_t updateTimeoutTimer = UPDATE_TIMEOUT;
volatile uint32_t commandTimer;
volatile uint32_t doorKeyTimer;
volatile uint32_t upTime;

#define MEMLOWERLIMIT (25 * 1000) // kB


void* timerThread(void* args) {
	int presc = 10;

	uint32_t memTotal;
	uint32_t memFree;
	uint32_t memAvailable;
	uint32_t memStartFree = 0;
	char buf[100];


	while(1){
		sleep(1);
		upTime++;
		if ( updateTimeoutTimer )
			updateTimeoutTimer--;
		if( commandTimer)
			commandTimer--;

		if ( overallTimeoutTimer )
			overallTimeoutTimer--;
		else
			exit(2);

		 if (doorKeyTimer )
			 doorKeyTimer--;

		if ( --presc == 0) {
			presc = 60;
			if  ( readValueFromFile("/proc/meminfo",buf, sizeof( buf)) == 0 ) {
				sscanf(buf,"MemTotal:%u kB\nMemFree: %u kB\nMemAvailable: %u" , &memTotal,&memFree,&memAvailable);

				if ( memFree < MEMLOWERLIMIT) {
					if (status == STATUS_IDLE)
						exit(3);  // reboot thru calling script (Q&D)
				}
				if ( memStartFree == 0 )
					memStartFree = memFree;
			}
		}
	}
	return ((void *)0);
}

