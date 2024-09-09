/*********************************************************************
 * i2c_funcs.h : I2C Access Functions
 *********************************************************************/
/*
 * Write 8 bits to I2C bus peripheral:
 */
int i2c_write8(int addr,int reg,int byte);
/*
 * Write 16 bits to Peripheral at address :
 */
int i2c_write16(int addr,int reg,int value);
	/*
 * Read 8-bit value from peripheral at addr :
 */
int i2c_read8(int addr,int reg);
/*
 * Read 16-bits of data from peripheral :
 */
int i2c_read16(int addr,int reg);
/*
 * Open I2C bus and check capabilities :
 */
int i2c_init(const char *node);
/*
 * Close the I2C driver :
 */
void i2c_close(void);

/*********************************************************************
 * End i2c_funcs.c - by Warren Gay
 * Mastering the Raspberry Pi - ISBN13: 978-1-484201-82-4
 * This source code is placed into the public domain.
 *********************************************************************/

