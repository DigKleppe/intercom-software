/*
 * connections.h
 *
 *  Created on: Feb 18, 2019
 *      Author: dig
 */

#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_


void startConnectionThreads( void);
extern uint16_t connectCntrBaseFloor, connectCntrFirstFloor;  // connections to doorstations
extern uint16_t connectCntrBaseFloor,connectCntrFirstFloor;
extern uint16_t ackCntrBaseFloor,ackCntrFirstFloor;
void UDPSend( floor_t);
void UDPsendMessage (char * message );
void UDPsendExtendedMessage (char * message );

#endif /* CONNECTIONS_H_ */
