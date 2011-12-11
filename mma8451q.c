//******************************************************************************
//   Telit Communications Manager for Project Bacon
//
//   Description: Initializes Telit GE865-QUAD and posts datapacket to server
//   USCI_A0 RX interrupt triggers fills circular buffer
//   ACLK = BRCLK = LFXT1 = 32768, MCLK = SMCLK = DCO~1048k
//   //* An external watch crystal is required on XIN XOUT for ACLK *//
//
//
//                MSP430xG461x
//             -----------------
//         /|\|              XIN|-
//          | |                 | 32kHz
//          --|RST          XOUT|-
//            |                 |
//            |     P4.7/UCA0RXD|------------>
//            |                 | 4800 - 8N1
//            |     P4.6/UCA0TXD|<------------
//
//   R. von Kurnatowski III
//   TX/RX Labs
//   November 2011
//   Built with CCE Version: 4.0.1
//******************************************************************************
#include "txrxlpf.h"
#include "msp430xG46x.h"
#include "mma8451q.h"
#include "txrxlpfi2c.h"



unsigned char smode[2] = {0x09, 0x80};
unsigned char srnge[2] = {0x0E, 0x00};
unsigned char sctl1[2] = {0x2A, 0x3E};
unsigned char sctl2[2] = {0x2B, 0x01};
unsigned char sctl4[2] = {0x2D, 0x40};
unsigned char sctl5[2] = {0x2E, 0x40};
//8 vs 14 bit data
// sample rate

void initMMA8451Q() {
	writeI2C(0x1c, smode, 2, 1); //0x09 1010 0000   A0         samples 10XX XXXX
	writeI2C(0x1c, srnge, 2, 1); //0x0E 0000 0000   00       range selection  0000 00 XX
	writeI2C(0x1c, sctl1, 2, 1); //0x2A 0011 1010   3A
	writeI2C(0x1c, sctl2, 2, 1); //0x2B 0001 1011   1B
	writeI2C(0x1c, sctl4, 2, 1); //0x2D 0100 0000   40
	writeI2C(0x1c, sctl5, 2, 1); //0x2E 0100 0000   40

}

void beginMMA8451Q () {

	unsigned char ctl1[] = {0x2A, 0x00};
	writeI2C(0x1c, ctl1, 1, 0); //0x2E 0100 0000  40
	readI2C(0x1c, &(ctl1[1]) , 1, 1);
	ctl1[1] |= 0x01;
	writeI2C(0x1c,ctl1, 2, 1);
}
unsigned char mma8451qVector(void) {
	return LPM3_bits;
}
void executeMMA8451Q(unsigned char* buffer, unsigned int length) {
	enableI2C(12);
	initMMA8451Q();
	attachInterrupt(mma8451qVector, 1,6, 1);
	beginMMA8451Q();
	LPM3;
	detachInterrupt(1,6);
	unsigned char statreg[2] = {0x00, 0x01};
	writeI2C(0x1c, statreg, 1, 0);
	readI2C(0x1c, statreg , 1, 1);
	writeI2C(0x1c, statreg+1, 1, 0);
	readI2C(0x1c, buffer , 96, 1);
	disableI2C();
	return;
}







