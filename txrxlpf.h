/*
 * txrxlpf.h
 *
 *  Created on: Oct 16, 2011
 *      Author: Peter
 */

#ifndef TXRXLPF_H_
#define TXRXLPF_H_

#define RLY1  BIT6
#define TRANSMIT_BUFFER_SIZE 204
#define PULSE_LENGTH 20 // pulse length in seconds

#define     Bitime_5              0x05*4                      // ~ 0.5 bit length + small adjustment
#define     Bitime                13*4//0x0D


typedef struct {
	char c;
} proc_struct_t;


typedef void (*procFcn)(proc_struct_t);
typedef unsigned char (*irqVector)(void);
typedef enum {
	STATE_INITIAL = 0,
    STATE_NORMAL,
    STATE_BATTERY_LOW
} control_state_t;

void attachInterrupt(irqVector vector, unsigned char port, unsigned char bit, unsigned char edge);
void detachInterrupt(unsigned char port, unsigned char bit);
unsigned char* getBuffer(unsigned int length);
unsigned int releaseBuffer(unsigned int length);
void logError();
void flushTransmission();
void closeTransmission();
void show_tick(void);
void executeProcs(void);
void configure_timer(void);
void initialize(void);
void enableSensors();


#endif /* TXRXLPF_H_ */
