/*
 * temp.c
 *
 *  Created on: Dec 27, 2011
 *      Author: Administrator
 */
#include "msp430xG46x.h"

void init() {
	// Setup Timer
	TACTL |= TACLR;
	TACTL |=  TASSEL_1;
	TACCTL1 |= CM_2 + CCIS_1 + CAP + CCIE;
	//setup comparator
	TACTL |= MC_2;

}
