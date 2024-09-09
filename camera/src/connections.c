/*
 * connections.c
 *
 *  Created on: Feb 17, 2019
 *      Author: dig
 */

#include "camera.h"
#include "timerThread.h"

#include <stdio.h>
#include <string.h>	//strlen
#include <sys/socket.h>
#include <unistd.h>	//write
#include <pthread.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
//#include <sys/types.h>
//#include <ifaddrs.h>
//#include <netinet/in.h>


//#define PRINTDEBUG 1

// receives from stations on port  RECEIVE_STATIONSPORT1/2
// args : pointer to addres ( 0 = base, 1 = first floor )

// receives ID ( house no) and state of hook and keys of station
// resets the timeoutTimer of station.
// returns the status of bellKey to client

int IDerrCntr;
uint32_t connectCntr;

volatile uint32_t commErrCntr[NR_STATIONS];
volatile uint32_t commOKCntr[NR_STATIONS];


void getMyIpAddress( char* dest) {
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
	sprintf(dest, "%s\n", inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));
}

// sends to monitor messages

void UDPsendMessage (char * message ) {
	int sock = 0;
	int opt = 1;

	int len = strlen( message);
//	if ( len > 1000)
//		return;

	struct sockaddr_in serv_addr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)   // udp
		printf("\n Socket creation error \n");
	else {
		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		//	serv_addr.sin_addr.s_addr = INADDR_ANY;

	//	serv_addr.sin_addr.s_addr = IPADDR_BROADCAST;// INADDR_ANY;
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

		inet_pton(AF_INET, "192.168.2.255", &serv_addr.sin_addr.s_addr);

		if ( myFloorID == BASE_FLOOR) {
			serv_addr.sin_port = htons(MONITORPORT1);
			//		sendto(sock, "0: ", 3, MSG_CONFIRM, (const struct sockaddr *) &serv_addr,sizeof(serv_addr));
		}
		else {
			serv_addr.sin_port = htons(MONITORPORT2);
			//		sendto(sock, "1: ", 3, MSG_CONFIRM, (const struct sockaddr *) &serv_addr,sizeof(serv_addr));

		}
		sendto(sock, (uint8_t *) message, len, MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		close ( sock );
	}
}

// acknowledge data to telephone

void UDPack( int stationID) {
	int sock = 0;
	struct sockaddr_in serv_addr,cliaddr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)   // udp
		printf("\n Socket creation error \n");
	else {
		memset(&serv_addr, '0', sizeof(serv_addr));

		inet_pton(AF_INET, BASE_IP_ADDRESS, &serv_addr.sin_addr);
		serv_addr.sin_addr.s_addr += (100 + stationID*2) << 24; // add destination to lsb ip
		serv_addr.sin_family = AF_INET;
		//	serv_addr.sin_addr.s_addr = INADDR_ANY;

		serv_addr.sin_port = htons(UDPACKPORT1);

//		if ( myFloorID == BASE_FLOOR) {
//			serv_addr.sin_port = htons(UDPACKPORT1);
//		}
//		else {
//			serv_addr.sin_port = htons(UDPACKPORT2);
//		}
		sendto(sock, (uint8_t *) &station[stationID],sizeof(station_t), MSG_CONFIRM, (const struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		close ( sock );
	}
}

// recieves alive messages with button info

void* UDPserverThread (void* args) {
	int socket_fd = 0, accept_fd = 0;
	uint32_t addr_size, sent_data;
	int count;
	threadStatus_t  * pThreadStatus = args;
	struct sockaddr_in sa, cliaddr;
	int stationID;
	receiveData_t tempRecData;

	char *buf;
	int buflen;
	bool err = false;
	int opt = 1;

	pThreadStatus = args;
	opt = 1;

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 2;
	receiving_timeout.tv_usec = 0;

	while (1) {
		err = false;
		if ( !updateInProgress) {

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

				setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout));

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
				int len = sizeof(cliaddr);
				int n;
				n = recvfrom(socket_fd,  (uint8_t *)&tempRecData , sizeof(tempRecData), MSG_WAITALL, ( struct sockaddr *) &cliaddr,  &len);
				if ( n > 0) {
					addr_size = sizeof(cliaddr);
					stationID = (inet_lnaof (cliaddr.sin_addr) -100 ) / 2; // = low byte address
					if (stationID < NR_STATIONS) {
						//	receiveData[ stationID] = tempRecData;
						//			printf ( "udp rec from %d: %5d %2d %2d\n", stationID * 2,   tempRecData.uptime , tempRecData.keys, tempRecData.softwareversion);
						memcpy( &station[stationID] ,&tempRecData, sizeof(station_t));
						timeoutTimer[stationID]= STATIONTIMEOUT;
						UDPack(stationID); // acknowlegde message
					}
				}
			}
			close ( socket_fd );
		}
		else
			sleep(1);
	}
	pThreadStatus->run = false;
//	pthread_exit(args);
	return ( args);
}

// sends alive messages to every telephone

void* UDPkeepAliveClient (void* args) {
	int sock = 0;
	struct sockaddr_in serv_addr;
	int destAddress = 2;

	while (1) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  // udp
			printf("\n Socket creation error \n");
			sleep(1);
		}
		else {
			memset(&serv_addr, '0', sizeof(serv_addr));

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(UDPKEEPALIVEPORT);

			inet_pton(AF_INET, BASE_IP_ADDRESS, &serv_addr.sin_addr);
			serv_addr.sin_addr.s_addr += (100 + destAddress) << 24; // add destination to lsb ip

			sendto(sock,  (uint8_t* )&destAddress , sizeof(destAddress) , MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof(serv_addr));
			close(sock);

		//	destAddress++;
			destAddress += 2;
			if ( destAddress > 62)
				destAddress = 2;
			if ( updateInProgress )
				usleep(500000);
			else
				usleep(20000);
		}
	}
}

//  when streaming TCP is very slow , UDP works fine
// dest address = low byte ip ( 2, 4 .. 60)

void UDPsendToTelephone (int destAddress, int port,  uint8_t *dataToTelephone, int txLen , uint8_t *dataFromTelephone, int rxLen ) {
	int sock = 0;
	struct sockaddr_in serv_addr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  // udp
		printf("\n Socket creation error \n");
		return;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(UDPCHATPORT2);
	inet_pton(AF_INET, BASE_IP_ADDRESS, &serv_addr.sin_addr);
	serv_addr.sin_addr.s_addr += (100 + destAddress) << 24; // add destination to lsb ip

	sendto(sock,  (uint8_t* )dataToTelephone , txLen , MSG_CONFIRM, (const struct sockaddr *) &serv_addr,sizeof(serv_addr));
	close(sock);
}

// used for update


// dest address = low byte ip ( 2, 4 .. 60)
int TCPsendToTelephone ( int destAddress, int port,  uint8_t *dataToTelephone, int txLen , uint8_t *dataFromTelephone, int rxLen ) {
	struct sockaddr_in address;
	int sock = 0, bytesread, res;
	struct sockaddr_in serv_addr;
	struct timeval receiving_timeout;

	if (timeoutTimer[destAddress/2] == 0  ) // do not send if destination is not present
		return (int) RESULT_ERR;

	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)  // tcp
	{
		printf("\n Socket creation error \n");
		return (int) RESULT_ERR;
	}

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout));

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, BASE_IP_ADDRESS, &serv_addr.sin_addr);
	serv_addr.sin_addr.s_addr += (100 + destAddress) << 24; // add destination to lsb ip

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		//		printf("sendToTelephone connection Failed \n");
		bytesread = (int) RESULT_ERR;
		commErrCntr[destAddress/2]++;
	}
	else {
		res = send(sock , (uint8_t* )dataToTelephone , txLen , 0 );

		bytesread = read( sock , dataFromTelephone, rxLen);

		timeoutTimer[destAddress/2] = STATIONTIMEOUT;
		commOKCntr[destAddress/2]++;
	}

	close(sock);
	return bytesread;
}




void startConnectionThreads( void){
	pthread_t ID1;
	pthread_t ID2;

	pthread_create(&ID1, NULL, &UDPkeepAliveClient, NULL);
	pthread_create(&ID2, NULL, &UDPserverThread, NULL);
	print("ConnectionThreads created successfully.\n");
}


