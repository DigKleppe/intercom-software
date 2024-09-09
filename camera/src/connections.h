/*
 * connections.h
 *
 *  Created on: Feb 18, 2019
 *      Author: dig
 */

#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_
#include <stdint.h>

int TCPsendToTelephone ( int destAddress, int port,uint8_t *dataToTelephone, int txLen , uint8_t *dataFromTelephone, int rxLen );
int UDPsendToTelephone (int destAddress, int port,  uint8_t *dataToTelephone, int txLen , uint8_t *dataFromTelephone, int rxLen );

void* serverThread(void* args);
extern uint32_t connectCntr;
extern volatile uint32_t commErrCntr[NR_STATIONS];
extern volatile uint32_t commOKCntr[NR_STATIONS];
void startConnectionThreads();
void UDPsendMessage (char * message);

void getMyIpAddress( char* dest);

#endif /* CONNECTIONS_H_ */
