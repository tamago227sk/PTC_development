#ifndef i2c_h
#define i2c_h

#include <cstdint>
#include <string>
#include <stdint.h>
#include "log.h"

typedef struct i2c_t {
	int fd; // file descriptor for the i2c device
	uint8_t slave; // the currently selected slave address for this i2c device
} i2c_t;


/* Init / Free */
int i2c_init(i2c_t *i2c, const char *device);
int i2c_free(i2c_t *i2c);

/* Basic read/write */
int i2c_read(i2c_t *i2c, uint8_t slave, uint8_t *buf, size_t len);
int i2c_write(i2c_t *i2c, uint8_t slave, uint8_t *buf, size_t len);

/* Write register, then read data */
int i2c_readwrite(i2c_t *i2c, uint8_t slave, uint8_t *rbuf, size_t rlen, uint8_t *wbuf, size_t wlen);
int i2c_writeread(i2c_t *i2c, uint8_t slave, uint8_t *rbuf, size_t rlen, uint8_t *wbuf, size_t wlen);

/* Register Access */
int i2c_reg_read(i2c_t *i2c, uint8_t slave, uint8_t reg);
int i2c_reg_write(i2c_t *i2c, uint8_t slave, uint8_t reg, uint8_t data);

#endif