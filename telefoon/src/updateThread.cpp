/*
 * updateThread.c
 *
 *  Created on: May 13, 2019
 *      Author: dig
 */

#include "telefoon.h"
#include "keys.h"
#include "timerThread.h"

#include <stdio.h>
#include <string.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>	//write
#include <stdlib.h>
#include <netinet/in.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <openssl/md5.h>

bool reboot;


unsigned char receivedMd5sum[MD5_DIGEST_LENGTH];
unsigned char calculatedMd5sum[MD5_DIGEST_LENGTH];
uint32_t fileLen;


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
	if(fstat(fd, &statbuf) < 0) exit(-1);
	return statbuf.st_size;
}

int getmd5(const char * fileName) {
	int file_descript;
	unsigned long file_size;
	char* file_buffer;

	file_descript = open(fileName, O_RDONLY);
	if(file_descript < 0) exit(-1);

	fileLen = get_size_by_fd(file_descript);
	//	printf("file size:\t%lu\n", file_size);

	file_buffer = (char * ) mmap(0, fileLen, PROT_READ, MAP_SHARED, file_descript, 0);
	MD5((unsigned char*) file_buffer, fileLen, calculatedMd5sum);
	munmap(file_buffer, fileLen);

	//	print_md5_sum(result);

	return 0;
}



int charpos( char *p, char ch){
	int max = strlen( p);
	int pos = 0;
	while ( p[pos] != ch && pos < max)
		pos++;
	if (pos == max)
		return -1;
	return pos;
}

FILE * openUpdateFile (){
	FILE *fptr;
	fptr = fopen("/root/tempFile","w");
	return fptr;
}

bool appendUpdateFile(FILE * fptr, char * receiveBuf, int count) {
	int len = fwrite (receiveBuf, 1, count, fptr);
	if (len == count)
		return 0;
	else
		return -1;
}

bool closeUpdateFile(FILE * fptr, char * newFileName) {
	fclose (fptr);
	getmd5("/root/tempFile");
	if (memcmp(calculatedMd5sum, receivedMd5sum, sizeof(receivedMd5sum)) == 0) {
		printf(" md5 ok\n");
		//	return rename( "tempFile" , newFileName);
		remove( newFileName);
		rename( "/root/tempFile" , newFileName);
		system("sync");

		if (strcmp( newFileName, "/root/telefoon") == 0) {
			system("chmod +x /root/telefoon");
		}
		return 0;
	}
	else {
		printf(" md5 failed\n");
		return -1;
	}

}

typedef enum { ERR_NONE, ERR_SOCK, ERR_FILE, ERR_FILELEN,  ERR_MD5 , ERR_BLOCK ,ERR_TIMEOUT } errUpdate_t;

// receives update messages from base station 100

void* updateServerThread (void* args) {
	int socket_fd = 0, accept_fd = 0;
	uint32_t addr_size, sent_data;
	int count;
	threadStatus_t * pThreadStatus =  (threadStatus_t *) args;
	char receiveBuf[1024];
	struct sockaddr_in sa, isa;
	int stationID;
	int state = 0;
	FILE * fptr= NULL;
	char newFileName[100];
	char newFileNameBuf[50];
	uint32_t fileLen;
	uint32_t md5sum;
	uint32_t receivedLen;
	int n;
	char *cp;

	char *buf;
	int buflen;
	errUpdate_t err;
	bool stop = false;
	bool updateError;
	int opt = 1;
	int blocks = 0;

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;

	while (!stop) {
		err = ERR_NONE;
		memset(&sa, 0, sizeof(struct sockaddr_in));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(UPDATEPORT);

		socket_fd = socket(PF_INET, SOCK_STREAM, 0);
		if (socket_fd < 0) {
			printf("socket call failed\n");
			err = ERR_SOCK;
		}
		if (!err) {
			if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
			{
				perror("setsockopt");
			}
			if (bind(socket_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
				perror("Bind to Port failed\n");
				err = ERR_SOCK;
			}
		}
		if (!err) {
			listen(socket_fd, 5);

			addr_size = sizeof(isa);
			accept_fd = accept(socket_fd, (struct sockaddr* ) &isa, &addr_size);

			stationID = (inet_lnaof (isa.sin_addr) -100 ); // = low byte address

			if (accept_fd < 0) {
				printf("accept failed\n");
				err = ERR_SOCK;
			}
			else {
				//	printf("Update accepted State:%d\n", state);
				setsockopt(accept_fd, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout));
				count = recv(accept_fd, (uint8_t *) &receiveBuf, sizeof(receiveBuf), 0);
				//	printf("rec %d, %d\n",count, state);
				printf(".");
				switch (state){  // first frame contents: "fileName=/nnnnnn/cccc" or "reboot"
				case 0:
					if ( (count > 3) && ( count < 100 )) {  // no response empty frame
						if( sscanf ( receiveBuf,"fileName=%[^;]s",newFileNameBuf) == 1) {
							printf("%s\n",newFileNameBuf);
							strcpy( newFileName, "/root/");
							strcat (newFileName,newFileNameBuf);
							receivedLen = 0;
							blocks = 0;
							cp = strstr(receiveBuf,";len=");
							if ( sscanf ( cp,";len=%d;", &fileLen) == 1){
								cp = strstr(receiveBuf,"md5=") + strlen( "md5=");
								memcpy (receivedMd5sum, cp, sizeof (receivedMd5sum));
								printf("MD5: %x  len:%d \n",receivedMd5sum,fileLen);

								fptr = openUpdateFile(); // make temporary file
								if (fptr) {
									state++;
									updateTimeoutTimer = UPDATE_TIMEOUT; // seconds
								}
								else {
									err = ERR_FILE;
									printf("updata err temp file\n");
								}
							}
						}
					}
					break;
				case 1:
					if (updateTimeoutTimer == 0 ){
						printf("update timeout\n");
						state = 0;
						err = ERR_TIMEOUT;
					}
					else {
						receivedLen += count;
						if (receivedLen > fileLen)
							err = ERR_FILELEN;
						else {
							if ( count > 0 ){
								appendUpdateFile(fptr, receiveBuf, count);
								blocks++;
							}
							else { // socket closed , end of file
								updateError = closeUpdateFile(fptr, newFileName); // check temporary file and rename if ok
								fptr = NULL;
								if (!updateError) {
									reboot = true;
									stop = true;
								}
								else {
									printf( " error MD5 len:%d blocks:%d\n",receivedLen, blocks);
									err = ERR_MD5;
								}
								state = 0;
							}
						}
					}
					break;
				}
				if( err > ERR_NONE  ){
					sprintf(receiveBuf,"Error %d", err);
					printf("Error %d\n", err);
				}
				else {
					if ( blocks == 0)
						sprintf(receiveBuf,"Update go");  // accept update
					else {
						if ( !reboot )
							sprintf(receiveBuf,"OK %d\n",blocks );
						else
							sprintf(receiveBuf,"Success");
					}

				}
				send(accept_fd , receiveBuf , strlen (receiveBuf) , 0 );
				close(accept_fd);
			}
			close(socket_fd);
			if( err){
				state = 0;
				if ( fptr )
					fclose (fptr);  //
				usleep(100000);
			}
		}
	}

	if ( reboot) {
		sleep(1);
		exit(EXIT_SUCCESS);  // reboot thru calling script (Q&D)
	}

	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}
