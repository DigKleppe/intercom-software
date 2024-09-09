/*
 * updateThread.c
 *
 *  Created on: May 13, 2019
 *      Author: dig
 */
//

#include "camera.h"
#include "connections.h"
#include "timerThread.h"

#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <openssl/md5.h>

enum { ERR_NONE, ERR_SOCK, ERR_FILE, ERR_FILELEN,  ERR_MD5 , ERR_BLOCK ,ERR_TIMEOUT } errUpdate_t;

#define RETRIES 5

unsigned char md5sum[MD5_DIGEST_LENGTH];
uint32_t fileLen;

uint32_t updateErrors[NR_STATIONS];

// Print the MD5 sum as hex-digits.
void print_md5_sum(unsigned char* md) {
	int i;
	for(i=0; i <MD5_DIGEST_LENGTH; i++) {
		printf("%02x",md[i]);
	}
}


// -lssl -lcrypto

// Get the size of the file by its file descriptor
unsigned long get_size_by_fd(int fd) {
	struct stat statbuf;
	if(fstat(fd, &statbuf) < 0)
		return -1;
	return statbuf.st_size;
}

int getmd5(char * fileName) {
	int file_descript;
	unsigned long file_size;
	char* file_buffer;

	file_descript = open(fileName, O_RDONLY);
	if(file_descript < 0)
		return -1;

	fileLen = get_size_by_fd(file_descript);
	if ( fileLen > 0 ) {
	//	printf("file size:\t%lu\n", file_size);
		file_buffer = mmap(0, fileLen, PROT_READ, MAP_SHARED, file_descript, 0);
		MD5((unsigned char*) file_buffer, fileLen, md5sum);
		munmap(file_buffer, fileLen);
	//	print_md5_sum(result);
		return 0;
	}
	else
		return -1;
}


bool transferFile ( char *fileName, int destAddress  ){
	FILE * fptr;
	char buffer[1024];
//	char buffer[200000];
	char respBuffer[50];
	int state = 0;
	bool err = false;
	//	uint32_t fileLen;
	//	uint32_t md5sum;
	uint32_t count;
	int n;
	int blocks = 0;
	int blocksResp;
	int errcode = 0;

	fptr = fopen( fileName,"r");
	if ( fptr == NULL){
		printf("\nFile open error %s\n",fileName);
		err = true;
	}
	while ( state < 2 && !err) {
		err = false;
		switch (state){
		case 0:
			// remove "updates/' from filename
			fileName += strlen("/root/updates/");
			n = sprintf( buffer,"fileName=%s;len=%d;md5=",fileName, fileLen);
			memcpy( &buffer[n],md5sum, sizeof(md5sum));
			n+= sizeof(md5sum);
			count = TCPsendToTelephone(destAddress,UPDATEPORT, buffer, n, respBuffer,sizeof(respBuffer)); // first frame
			if (sscanf(respBuffer,"Error %d", &errcode  ) > 0){
				err = true;
				printf("Error %d received\n",errcode );
				sprintf (message+ strlen( message),"Error %d received\n",errcode);
				state = 0;
			}
			else {
				if ( count > 0 ) {
					if (strstr(respBuffer,"Update go" ) > 0)
						state++;
					else
						err = true;
				}
				else
					err = true;
			}
			break;
		case 1:
			blocks++;
			count = fread(buffer,1,sizeof(buffer),fptr);
			if ( count < sizeof(buffer))
				printf("Last count: %d\n",count );

			n = TCPsendToTelephone(destAddress,UPDATEPORT, buffer , count, respBuffer,sizeof(respBuffer));
			if ( count > 0 && n <= 0) {
				err = true;  // error if count = 0
				printf("\nerror ");
				printf("no response\n");
				sprintf (message+ strlen( message),"no response\n");
			}
			if (sscanf(respBuffer,"Error %d", &errcode  ) > 0){
				err = true;
				printf("Error %d received\n",errcode );
				sprintf (message+ strlen( message),"Error %d received\n",errcode);
			}
			else {
				if ( sscanf(respBuffer,"OK %d",&blocksResp)){
					if (blocksResp != blocks ){
						err = true;
						printf("Block error send: %d rec: %d\n",blocks,blocksResp );
						sprintf (message+ strlen( message),"Block error send %d rec %d\n",blocks,blocksResp );
					}
				}
				else {
					err = true;
					printf("Response failed\n");
					sprintf (message+ strlen( message),"Response failed\n");
				}
			}
			if ( count == 0) {  // file transferred
				state = 2;
				if (strstr(respBuffer,"Success") >= 0){
					printf("\n%d success blocks send\n", blocks);
					sprintf (message+ strlen( message),"Success ");
					err = false;
				}
				else {
					printf("\n%d failed blocks send\n", blocks);
					sprintf (message+ strlen( message),"\nFailed");
				}
			}
			break;
		case 2:
			break;

		}
	}
	if ((int) fptr > 0)
		fclose (fptr);
	return err;
}


//// test
//void saveSettings() {
//
//	FILE *fptr;
//	fptr = fopen("/root/updates/test.tx","w");
//	if(fptr == NULL)
//	{
//		printf("File write open Error!\n");
//		return;
//	}
//	fprintf(fptr,"ringvolume");
//	fclose(fptr);
//	return;
//}


void* updateThread(void* args) {
	int n;
	FILE * fptr;
	int updateSoftwareversion = 0;
	bool err = false;
	threadStatus_t  * pThreadStatus = args;
	pThreadStatus->run = true;
	pThreadStatus->mustStop = false;
	char buffer[20];
	printf("updateThread started\n");
	bool allLastSoftware = false;
	int retries = 0;

//	saveSettings();

	fptr = fopen("/root/updates/versie.txt","r");
	if (fptr){
		fscanf(fptr, "versie:%d",&updateSoftwareversion); // read software version no
		fclose (fptr);
	}
	if (getmd5("/root/updates/telefoon") == -1 ) {
		printf( "No updates found\n");
		usleep ( 10000);
		pThreadStatus->run = false;
		while (1)
			sleep(100);

	//	pthread_exit(args);
	//	return ( NULL);
	}

	while (1) {  // check softwareversions
		allLastSoftware = true;
		for ( n = 1; n < sizeof(station)/sizeof( station_t); n++) {
			if (timeoutTimer[n] > 0) {
				if ( station[n].softwareversion != updateSoftwareversion &&  (updateErrors[n] < RETRIES) ){
					updateInProgress = true;
					allLastSoftware = false;
					usleep(100000);
					printf("sending update to no %d ", n * 2);
				//	sprintf (message,"sending update to no %d \n", n * 2); // print on screen

					err = transferFile("/root/updates/telefoon", n * 2); // send file to station
					if ( !err) {
						updateErrors[n] = 0;
						printf("reboot no %d\r\n", n * 2);
					//	sprintf (message+ strlen( message),"reboot no %d\r\n", n * 2);
						//						sprintf( buffer,"reboot");// give reboot command to station
						//						TCPsendToTelephone ( n* 2,UPDATEPORT, buffer, strlen(buffer), buffer, 10);
						station[n].softwareversion = updateSoftwareversion;  // will be overwritten with actual softwareversion
						usleep(100000);
						timeoutTimer[n] = 0;
					}
					else {
						printf("update failed no %d %d\r\n", n * 2,updateErrors[n]);
						updateErrors[n]++;
					//	sprintf (message + strlen( message),"update failed no %d %d\r\n", n * 2,updateErrors[n]); // print on screen

					}
				}
			}
		}
		if ( allLastSoftware)
			updateInProgress = false;
		sleep (2);
	}

	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}


