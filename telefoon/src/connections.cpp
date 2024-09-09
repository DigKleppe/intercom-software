/*
 * connections.c
 *
 *  Created on: Feb 17, 2019
 *      Author: dig
 */

#include "telefoon.h"
#include "keys.h"
#include "timerThread.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>	//write
#include <stdlib.h>
#include <netinet/in.h>

#define PCADDRESS "192.168.2.6"

int IDerrCntr;
uint16_t connectCntrBaseFloor,connectCntrFirstFloor;
uint16_t ackCntrBaseFloor,ackCntrFirstFloor;
int servConnectCtr1, servConnectCtr2;
extern uint32_t ackBaseFloorTimer,ackFirstFloorTimer;

// sends to monitor messages

void UDPsendMessage (char * message ) {
	int sock = 0;
	int opt = 1;
	struct sockaddr_in serv_addr,cliaddr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)   // udp
		printf("\n Socket creation error \n");
	else {
		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		//	serv_addr.sin_addr.s_addr = INADDR_ANY;
		inet_pton(AF_INET, "192.168.2.255", &serv_addr.sin_addr.s_addr);
	    setsockopt(sock, SOL_SOCKET, SO_BROADCAST ,&opt, sizeof(opt));
		serv_addr.sin_port = htons(MONITORTELEPHONESPORT);

		sendto(sock, (uint8_t *) message, strlen(message), MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		close ( sock );
	}
}


void UDPsendExtendedMessage (char * message ) {
	int sock = 0;
	struct sockaddr_in serv_addr,cliaddr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)   // udp
		printf("\n Socket creation error \n");
	else {
		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		//	serv_addr.sin_addr.s_addr = INADDR_ANY;
		inet_pton(AF_INET, "192.168.2.6", &serv_addr.sin_addr.s_addr);
		serv_addr.sin_port = htons(6000+houseNo);  // send to port 6002--6060

		sendto(sock, (uint8_t *) message, strlen(message), MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		close ( sock );
	}
}



// keepalive slow

void* UDPclientThread (void* args) {
	struct sockaddr_in address;
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};
	int written;
	bool checkBaseFloor = false;
	int n, len;

	while (1) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  // udp
			printf("\n Socket creation error \n");

		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(UDPCHATPORT);
		inet_pton(AF_INET, IPADDRESS_BASEFLOOR, &serv_addr.sin_addr);

		sendto(sock, (uint8_t *) &transmitData, sizeof(transmitData), MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		close(sock);

		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  // udp
			printf("\n Socket creation error \n");

		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(UDPCHATPORT);
		inet_pton(AF_INET, IPADDRESS_FIRSTFLOOR, &serv_addr.sin_addr);

		sendto(sock, (uint8_t *) &transmitData, sizeof(transmitData), MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
				sizeof(serv_addr));

		close(sock);

		usleep(100 * 1000);

	}
	return 0;
}

// sends transmitdata on UDPCHATPORT2

void UDPSend (floor_t  floor) {
	struct sockaddr_in address;
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};
	int written;
	bool checkBaseFloor = false;
	// sends to monitor messages

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  // udp
		printf("\n Socket creation error \n");
		return;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(UDPCHATPORT2);

	if ( floor == BASE_FLOOR) {
		inet_pton(AF_INET, IPADDRESS_BASEFLOOR, &serv_addr.sin_addr);
		transmitData.echoedCommand = receiveDataBaseFloor.command;  //echo back
	}
	else {
		inet_pton(AF_INET, IPADDRESS_FIRSTFLOOR, &serv_addr.sin_addr);
		transmitData.echoedCommand = receiveDataFirstFloor.command;
	}

	transmitData.seqNo++;
	sendto(sock, (uint8_t *) &transmitData, sizeof(transmitData), MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
			sizeof(serv_addr));
	close(sock);
}


void * UDPKeepAliveServerThread ( void * args) {

	int socket_fd = 0, accept_fd = 0;
	uint32_t addr_size, sent_data;
	int count;
	threadStatus_t  * pThreadStatus = (threadStatus_t *)  args;
	receiveData_t receiveData;

	struct sockaddr_in sa, cliaddr;
	int stationID;

	char *buf;
	int buflen;
	bool err = false;
	int opt = 1;

//	pThreadStatus = args;
	opt = 1;

	while (1) {
		err = false;

		memset(&sa, 0, sizeof(struct sockaddr_in));
		memset(&cliaddr, 0, sizeof(struct sockaddr_in));

		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(UDPKEEPALIVEPORT);

		socket_fd = socket(PF_INET, SOCK_DGRAM,0);
		if (socket_fd < 0) {
			printf("socket call failed\n");
			err = true;
		}
		if (!err) {
			if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
			{
				perror("setsockopt");
			}
			if (bind(socket_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
				perror("Bind to Port failed\n");
				err = true;
			}
		}
		if (!err) {
		//	int len = sizeof(cliaddr);
			socklen_t len = sizeof(cliaddr);
			int n;
			n = recvfrom(socket_fd,  (uint8_t *)&receiveData , sizeof(receiveData), MSG_WAITALL, ( struct sockaddr *) &cliaddr,  &len);

			if ( n > 0) {
				stationID = (inet_lnaof (cliaddr.sin_addr) -100 ); // = low byte address
				if ( stationID== 0) { // base floor
					connectCntrBaseFloor++;
				}
				else {
					connectCntrFirstFloor++;
				}
			}
		}
		close(socket_fd);
	}
	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}

// reads receivedata on UDPCHATPORT

void * UDPserverThread ( void * args) {

	int socket_fd = 0, accept_fd = 0;
	uint32_t addr_size, sent_data;
	int count;
	threadStatus_t  * pThreadStatus = (threadStatus_t  * )args;
	receiveData_t receiveData;

	//	char receiveBuf[1024];
	struct sockaddr_in sa, cliaddr;
	int stationID;

	char *buf;
	int buflen;
	bool err = false;
	int opt = 1;

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 2;
	receiving_timeout.tv_usec = 0;

	while (1) {
		err = false;

		memset(&sa, 0, sizeof(struct sockaddr_in));
		memset(&cliaddr, 0, sizeof(struct sockaddr_in));

		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(UDPCHATPORT2);

		socket_fd = socket(PF_INET, SOCK_DGRAM,0);
		if (socket_fd < 0) {
			printf("socket call failed\n");
			err = true;
		}
		if (!err) {
			if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
			{
				perror("setsockopt");
			}
			if (bind(socket_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
				perror("Bind to Port failed\n");
				err = true;
			}
		}
		if (!err) {
			setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,sizeof(receiving_timeout));
			socklen_t len = sizeof(cliaddr);
			int n;

			n = recvfrom(socket_fd,  (uint8_t *)&receiveData , sizeof(receiveData), MSG_WAITALL, ( struct sockaddr *) &cliaddr,  &len);

			if ( n > 0) {
				addr_size = sizeof(cliaddr);
				stationID = (inet_lnaof (cliaddr.sin_addr) -100 ); // = low byte address

				if ( stationID== 0) { // base floor
					receiveDataBaseFloor = receiveData;
					connectCntrBaseFloor++;
				}
				else {
					receiveDataFirstFloor = receiveData;
					connectCntrFirstFloor++;
				}
			}
		}
		close(socket_fd);
	}

	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}


void * UDPack1Thread ( void * args) {

	int socket_fd = 0, accept_fd = 0;
	uint32_t addr_size, sent_data;
	int count;
	threadStatus_t  * pThreadStatus = (threadStatus_t  * )args;
//	receiveData_t receiveData;
	transmitData_t receiveData;  // data received is dataTransmitted

	struct sockaddr_in sa, cliaddr;
	int stationID;

	char *buf;
	int buflen;
	bool err = false;
	int opt = 1;

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 2;
	receiving_timeout.tv_usec = 0;

	while (1) {
		err = false;

		memset(&sa, 0, sizeof(struct sockaddr_in));
		memset(&cliaddr, 0, sizeof(struct sockaddr_in));

		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(UDPACKPORT1);

		socket_fd = socket(PF_INET, SOCK_DGRAM,0);
		if (socket_fd < 0) {
			printf("socket call failed\n");
			err = true;
		}
		if (!err) {
			if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
			{
				perror("setsockopt");
			}
			if (bind(socket_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
				perror("Bind to Port failed\n");
				err = true;
			}
		}
		if (!err) {
			setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,sizeof(receiving_timeout));
			socklen_t len = sizeof(cliaddr);
			int n;

			n = recvfrom(socket_fd,  (uint8_t *)&receiveData , sizeof(receiveData), MSG_WAITALL, ( struct sockaddr *) &cliaddr,  &len);

			if ( n > 0) {
				addr_size = sizeof(cliaddr);

				if (receiveData.keys == transmitData.keys ) {
					stationID = (inet_lnaof (cliaddr.sin_addr) -100	 ); // = low byte address
					if ( stationID== 0) { // base floor
						ackCntrBaseFloor++;
						ackBaseFloorTimer = COMMANDTIMEOUT;
					}
					else {
						ackCntrFirstFloor++;
						ackFirstFloorTimer = COMMANDTIMEOUT;
					}
				}
			}
		}
		close(socket_fd);
	}

	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}




void startConnectionThreads( void){
	pthread_t ID1;
	pthread_t ID2;
	pthread_t ID3;
	pthread_t ID4;

	int result;

	result = pthread_create(&ID1, NULL, &UDPserverThread, NULL);
	if (result == 0)
		printf("UDPserverThread created successfully.\n");
	else {
		printf("UDPserverThread not created.\n");
	}

	result = pthread_create(&ID2, NULL, &UDPclientThread, NULL);
	if (result == 0)
		printf("UDPclientThread created successfully.\n");
	else {
		printf("UDPclientThread not created.\n");
	}

	result = pthread_create(&ID3, NULL, &UDPKeepAliveServerThread, NULL);
	if (result == 0)
		printf("UDPKeepAliveServerThread created successfully.\n");
	else {
		printf("UDPKeepAliveServerThread not created.\n");
	}

	result = pthread_create(&ID4, NULL, &UDPack1Thread, NULL);
	if (result == 0)
		printf("UDPack1Thread created successfully.\n");
	else {
		printf("UDPack1Thread not created.\n");
	}


}

