/*
 * io.c
 *
 *  Created on: Apr 4, 2019
 *      Author: dig
 */
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>

#include "io.h"

//extern void clearLastError();
//extern void setLastError(const char *fmt, ...);

//int writeValueToFile(char* fileName, char* buff);
//int writeIntValueToFile(char* fileName, int value);
//int readValueFromFile(char* fileName, char* buff, int len);
//int readIntValueFromFile(char* fileName);

//#define GPIO_FILENAME_DEFINE(pin,field) char fileName[255] = {0}; \
//sprintf(fileName, "/sys/devices/virtual/gpio/gpio%d/%s", pin, field);

#define GPIO_FILENAME_DEFINE(pin,field) char fileName[255] = {0}; \
		sprintf(fileName, "/sys/class/gpio/gpio%d/%s", pin, field);


#define setLastError printf


int writeValueToFile(const char* fileName, const char* buff) {
	FILE *fp = fopen(fileName,"w");
	if (fp == NULL) {
		setLastError("Unable to open file %s\n",fileName);
		printf("Unable to open file %s\n",fileName);
		return -1;
	} else {
		fwrite ( buff, strlen(buff), 1, fp );
	}
	fclose(fp);
	return 0;
}


int writeIntValueToFile(const char* fileName, int value) {
	char buff[50];
	sprintf(buff, "%d", value);
	writeValueToFile(fileName, buff); // not always ok in one time ???
	usleep(10000);
	return writeValueToFile(fileName, buff);
}


int readValueFromFile(const char* fileName, char* buff, int len) {
	int ret = -1;
	FILE *fp = fopen(fileName,"r");
	if (fp == NULL) {
		setLastError("Unable to open file %s\n",fileName);
		return -1;
	} else {
		if (fread( (void *) buff, sizeof(char), len, fp)>0) {
			ret = 0;
		}
	}
	fclose(fp);
	return ret;
}


int readIntValueFromFile(const char* fileName) {
	char buff[255];
	memset(buff, 0, sizeof(buff));
	int ret = readValueFromFile(fileName, buff, sizeof(buff)-1);
	if (ret == 0) {
		return atoi(buff);
	}
	return ret;
}

/*
nanopi display:
GPIOA0: 0
GPIOB0: 32
GPIOC0: 64
GPIOD0: 96
GPIOE0: 128
GPIOF0: 160
GPIOG0: 192
GPIOH0: 224
GPIOJ0: 256
GPIOK0: 288
GPIOL0: 320
GPIOM0: 352
 */


static int exportGPIOPin(int pin)
{
	return writeIntValueToFile("/sys/class/gpio/export", pin);
}

//static int unexportGPIOPin(int pin)
//{
//	return writeIntValueToFile("/sys/class/gpio/unexport", pin);
//}

static int getGPIOValue(int pin)
{
	GPIO_FILENAME_DEFINE(pin, "value")
  	return readIntValueFromFile(fileName);
}

//static int setGPIOValue(int pin, int value)
//{
//	GPIO_FILENAME_DEFINE(pin, "value")
//  	return writeIntValueToFile(fileName, value);
//}

static int setGPIODirection(int pin, int direction)
{
	GPIO_FILENAME_DEFINE(pin, "direction")
    		char directionStr[10];
	if (direction == GPIO_IN) {
		strcpy(directionStr, "in");
	} else if (direction == GPIO_OUT) {
		strcpy(directionStr, "out");
	} else {
		return -1;
	}
	return writeValueToFile(fileName, directionStr);
}

//static int getGPIODirection(int pin)
//{
//	GPIO_FILENAME_DEFINE(pin, "direction")
//    		char buff[255] = {0};
//	int direction;
//	int ret = readValueFromFile(fileName, buff, sizeof(buff)-1);
//	if (ret >= 0) {
//		if (strncasecmp(buff, "out", 3)==0) {
//			direction = GPIO_OUT;
//		} else if (strncasecmp(buff, "in", 2)==0) {
//			direction = GPIO_IN;
//		} else {
//			return -1;
//		}
//		return direction;
//	}
//	return ret;
//}

void initIo() {
	exportGPIOPin(P1);
	setGPIODirection(P1,GPIO_IN);
	exportGPIOPin(P2);
	setGPIODirection(P2,GPIO_IN);
	exportGPIOPin(P3);
	setGPIODirection(P3,GPIO_IN);
	exportGPIOPin(HANDSET);
	setGPIODirection(HANDSET,GPIO_IN);
}

int getSwitches( void ){
	int sw1 = getGPIOValue(P1);
	int sw2 = getGPIOValue(P2);
	int sw3 = getGPIOValue(P3);
	int sw4 = getGPIOValue(HANDSET);
	return (~(sw1 + (sw2<<1) + (sw3<<2) + (sw4<<3))) & 0B1111; // invert
}
