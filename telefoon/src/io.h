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

// switches
#define P1			(58)
#define P2			(59)
#define P3			(62)
#define HANDSET		(63)

#define P1MASK 		(1)
#define P2MASK 		(2)
#define P3MASK 		(4)
#define HANDSETMASK	(8)

void initIo();
int getSwitches( void );
int writeIntValueToFile(const char* fileName, int value);
int writeValueToFile(const char* fileName, const char* buff);
int readValueFromFile(const char* fileName, char* buff, int len);

#endif /* IO_H_ */
