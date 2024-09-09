/*
 * I2Cwrapper.h
 *
 *  Created on: Mar 17, 2019
 *      Author: dig
 */

#ifndef I2CWRAPPER_H_
#define I2CWRAPPER_H_

int I2CWrapperOpen(int BUS, int SlaveAddress);
int I2CWrapperSlaveAddress(int handle, int SlaveAddress);
int I2CWrapperReadBlock(int handle, unsigned char cmd, unsigned char  size,  void * array);
int I2CWrapperReadByte(int handle, unsigned char cmd);
int I2CWrapperWriteByte(int handle,unsigned char cmd, unsigned char value);
int I2CWrapperReadWord(int handle, unsigned char cmd);
int I2CWrapperWriteWord(int handle,unsigned char cmd, unsigned short value);

#endif /* I2CWRAPPER_H_ */
