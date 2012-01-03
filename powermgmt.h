/*
 * mma8451q.h
 *
 *  Created on: Dec 3, 2011
 *      Author: Administrator
 */

#ifndef MMA8451Q_H_
#define MMA8451Q_H_


#define PM_SYSPWR 0x04
#define PM_SRVPWR 0x08
#define PM_ENABLE 0xFF
#define PM_DISABLE 0x00
#define MAX_BATT_V 0xFFF
#define MIN_BATT_V 0x200
#define BATT_LOW  1
#define BATT_FULL 2
#define BATT_CHRGNG 0

void setPwrBusStatus(unsigned char bus, unsigned char state);
unsigned char manageBatt();
#endif /* POWERMGMT_H_ */
