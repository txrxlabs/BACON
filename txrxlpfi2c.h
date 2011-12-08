/*
 * txrxlpfi2c.h
 *
 *  Created on: Dec 6, 2011
 *      Author: Administrator
 */

#ifndef TXRXLPFI2C_H_
#define TXRXLPFI2C_H_

#define SDA_PIN 0x02                                  // msp430x261x UCB0SDA pin
#define SCL_PIN 0x04

void enableI2C(unsigned char prescaler);
void disableI2C();
void writeI2C(unsigned char slave_address, unsigned char *bytes, unsigned char count, unsigned char terminate);
void readI2C(unsigned char slave_address, unsigned char *buffer, unsigned int length, unsigned char terminate);

#endif /* TXRXLPFI2C_H_ */
