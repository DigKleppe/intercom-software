/*
 * io.h
 *
 *  Created on: Apr 4, 2019
 *      Author: dig
 */

#ifndef IO_H_
#define IO_H_

// Direction
#define GPIO_IN   (1)
#define GPIO_OUT  (2)

// Value
#define GPIO_LOW  (0)
#define GPIO_HIGH (1)


// GPIO numbers

#define SW1			(59)
#define SW2			(58)
#define MAGNET		(62)
#define CAMERALEDS	(63)

void initIo();
void setCameraLEDS( bool state);
void setDooropen( bool state);
int getPushButtons( void );
int writeIntValueToFile(char* fileName, int value);
int writeValueToFile(char* fileName, char* buff);
int readValueFromFile(char* fileName, char* buff, int len);



#endif /* IO_H_ */
