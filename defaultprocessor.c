/*
 * SampleProcess.c
 *
 *  Created on: Dec 5, 2011
 *      Author: Administrator
 */

#include "defaultprocessor.h"
#include "mma8451q.h"

procFcn procMap[] = {mma8451qProc, thermistorProc};

void mma8451qProc(proc_struct_t proc_struct) {
	unsigned char *buff = getBuffer(96+7);//get data from sensor
	buff[0] = 0x00;
	buff[1] = 0x10;
	buff[2] = 0x00;

	buff[3] = 0x00;
	buff[4] = 0x00;
	buff[5] = 0x00;
	buff[6] = 0x60;

	executeMMA8451Q(buff+7, 96);
	return;
}

void thermistorProc(proc_struct_t proc_struct) {
	//get data from sensor
	// manipulate
	//allocate space
	// vopy into transmit buffer
	//return
}

