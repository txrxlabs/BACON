/*
 * txrxlpfi2c.c
 *
 *  Created on: Dec 5, 2011
 *      Author: Administrator
 */
#include <msp430fg4619.h>
#include "txrxlpfi2c.h"
void enableI2C(unsigned char prescaler) {
	P3SEL |= SDA_PIN + SCL_PIN;                 // Assign I2C pins to USCI_B0
	UCB0CTL1 = UCSWRST;                        // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;       // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;              // Use SMCLK, keep SW reset
	UCB0BR0 = prescaler;                         // set prescaler
	UCB0BR1 = 0;
	UCB0CTL1 &= ~UCSWRST;                       // Clear SW reset, resume operation
	UCB0I2CIE = UCNACKIE;
}

void disableI2C() {
	UCB0CTL1 = UCSWRST;
}

void writeI2C(unsigned char slave_address, unsigned char *bytes, unsigned char count, unsigned char terminate) {
	UCB0I2CSA = slave_address;                  // Set slave address
	UCB0CTL1 |= UCTR + UCTXSTT;                 // I2C TX, start condition
	 // wait for start to complete
	while(count >0) {
		UCB0TXBUF = *(bytes++);                            //write tx buffer
		while((UCB0CTL1 & UCTXSTT)) __no_operation();
		while(!(IFG2 & UCB0TXIFG)) {__no_operation();} // wait for tx buffer to be empty
		count--;                                       // dec bytes remaining to send
	}
	if(terminate) {
		UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
		while(UCB0CTL1 & UCTXSTP) {__no_operation();}  // wait for I2C stop to be sent
	}
}

void readI2C(unsigned char slave_address, unsigned char *buffer, unsigned int length, unsigned char terminate)
{
	UCB0I2CSA = slave_address;                  // Set slave address
	UCB0CTL1 &= ~UCTR;
	UCB0CTL1 |= UCTXSTT;                 // I2C TX, start condition
	while(UCB0CTL1 & UCTXSTT) {__no_operation();}
	while(length>0) {
		if(length==1)UCB0CTL1 |= UCTXSTP;                    // set I2C stop condition for NACK
		while(!(IFG2 & UCB0RXIFG)) {__no_operation();}       // wait for RX Buffer to be full
		while(UCB0CTL1 & UCTXSTP) {__no_operation();}        // wait for Stop if set
		*buffer = UCB0RXBUF;     							  // read buffer
		buffer++;
		length--;
	}
}
