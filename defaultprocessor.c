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
	unsigned char *buff = getBuffer(96);//get data from sensor
	executeMMA8451Q(buff, 96);
	return;
}

void thermistorProc(proc_struct_t proc_struct) {
	//get data from sensor
	// manipulate
	//allocate space
	// vopy into transmit buffer
	//return
}

