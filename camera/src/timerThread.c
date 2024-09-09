/*
 * timerThread.c
 *
 *  Created on: May 5, 2019
 *      Author: dig
 */

#include "camera.h"
#include "io.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t upTime;
extern bool active;

uint32_t activeTimer[NR_STATIONS];
uint32_t timeoutTimer[NR_STATIONS];
uint32_t commandTimer[NR_STATIONS];
uint32_t openDoorTimer;

#define MEMLOWERLIMIT (25 * 1000) // kB

void* timerThread(void* args) {
	int n;
	int presc = 10;
	uint32_t memTotal;
	uint32_t memFree;
	uint32_t memAvailable;
	uint32_t memStartFree = 0;
	char buf[100];
	int len;

	while(1){
		upTime++;
		for ( int n = 1; n < NR_STATIONS; n++) {
			if ( timeoutTimer[n] > 0 )
				timeoutTimer[n]--;
			if ( activeTimer[n] > 0)
				activeTimer[n]--;
			if( commandTimer[n] >0)
				commandTimer[n]--;
		}

		if (openDoorTimer > 0) {
			openDoorTimer--;
			if ( openDoorTimer == 0 )
				setDooropen( false);
		}

		if ( --presc == 0) {
			presc = 60;
			if  ( readValueFromFile("/proc/meminfo",buf, sizeof( buf)) == 0 ) {
				len = sscanf(buf,"MemTotal:%u kB\nMemFree: %u kB\nMemAvailable: %u" , &memTotal,&memFree,&memAvailable);

				if ( memFree < MEMLOWERLIMIT) {
					if (!active)
						exit(EXIT_FAILURE);  // reboot thru calling script (Q&D)
				}
				if ( memStartFree == 0 )
					memStartFree = memFree;
			}
		}
		sleep(1);
	}
	return ((void *)0);
}

