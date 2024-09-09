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
#include <strings.h>
#include "io.h"
#include "camera.h"

//extern void clearLastError();
//extern void setLastError(const char *fmt, ...);

int writeValueToFile(char* fileName, char* buff);
int writeIntValueToFile(char* fileName, int value);
int readValueFromFile(char* fileName, char* buff, int len);
int readIntValueFromFile(char* fileName);

//#define GPIO_FILENAME_DEFINE(pin,field) char fileName[255] = {0}; \
//sprintf(fileName, "/sys/devices/virtual/gpio/gpio%d/%s", pin, field);

#define GPIO_FILENAME_DEFINE(pin,field) char fileName[255] = {0}; \
sprintf(fileName, "/sys/class/gpio/gpio%d/%s", pin, field);


#define setLastError printf


int writeValueToFile(char* fileName, char* buff) {
    FILE *fp = fopen(fileName,"w");
    if (fp == NULL) {
       setLastError("Unable to open file %s\n",fileName);
       return -1;
    } else {
        fwrite ( buff, strlen(buff), 1, fp );
    }
    fclose(fp);
    return 0;
}


int writeIntValueToFile(char* fileName, int value) {
    char buff[50];
    sprintf(buff, "%d", value);
    return writeValueToFile(fileName, buff);
}


int readValueFromFile(char* fileName, char* buff, int len) {
    int ret = -1;
    FILE *fp = fopen(fileName,"r");
    if (fp == NULL) {
        setLastError("Unable to open file %s\n",fileName);
        return -1;
    } else {
        if (fread(buff, sizeof(char), len, fp)>0) {
            ret = 0;
        }
    }
    fclose(fp);
    return ret;
}


int readIntValueFromFile(char* fileName) {
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


int exportGPIOPin(int pin)
{
   return writeIntValueToFile("/sys/class/gpio/export", pin);
}

int unexportGPIOPin(int pin)
{
    return writeIntValueToFile("/sys/class/gpio/unexport", pin);
}

int getGPIOValue(int pin)
{
    GPIO_FILENAME_DEFINE(pin, "value")
    return readIntValueFromFile(fileName);
}

int setGPIOValue(int pin, int value)
{
    GPIO_FILENAME_DEFINE(pin, "value")
    return writeIntValueToFile(fileName, value);
}

int setGPIODirection(int pin, int direction)
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

int getGPIODirection(int pin)
{
    GPIO_FILENAME_DEFINE(pin, "direction")
    char buff[255] = {0};
    int direction;
    int ret = readValueFromFile(fileName, buff, sizeof(buff)-1);
    if (ret >= 0) {
        if (strncasecmp(buff, "out", 3)==0) {
            direction = GPIO_OUT;
        } else if (strncasecmp(buff, "in", 2)==0) {
            direction = GPIO_IN;
        } else {
            return -1;
        }
        return direction;
    }
    return ret;
}

void initIo() {
	exportGPIOPin(CAMERALEDS);
	setGPIODirection(CAMERALEDS,GPIO_OUT);
	exportGPIOPin(MAGNET);
	setGPIODirection(MAGNET,GPIO_OUT);
	exportGPIOPin(SW1);
	setGPIODirection(SW1,GPIO_IN);
	exportGPIOPin(SW2);
	setGPIODirection(SW2,GPIO_IN);

}

void setCameraLEDS( bool state) {
	setGPIOValue(CAMERALEDS, (int) state );
}

void setDooropen( bool state){
	char mssg[20];
	print("Door: %d\n", (int )state);
	setGPIOValue(MAGNET, (int) state );

}

int getPushButtons( void ){
	int sw1 = getGPIOValue(SW1);
	int sw2 = getGPIOValue(SW2);
	return (~(sw1 + (sw2<<1))) & 0B11; // invert
}


