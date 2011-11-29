/*
 * txrxlpf.h
 *
 *  Created on: Oct 16, 2011
 *      Author: Peter
 */

#ifndef TXRXLPF_H_
#define TXRXLPF_H_

#define RLY1  BIT6;

#define PULSE_LENGTH 1 // pulse length in minutes
#define MINUTE_LENGTH 10 // length of a minute in seconds
#define DATASTORE_MAX_SIZE 3 // number of slots in data store

#define THERMOMETER_ID 0x07

//   Conditions for 9600/4=2400 Baud SW UART, SMCLK = 1MHz
#define     Bitime_5              0x05*4                      // ~ 0.5 bit length + small adjustment
#define     Bitime                13*4//0x0D


typedef enum {
	STATE_INITIAL = 0,
    STATE_NORMAL,
    STATE_BATTERY_LOW
} control_state_t;

typedef struct {
	char instrument_id;
	long data;
} entry_t;

typedef struct {
	unsigned char front;
	unsigned char rear;
	unsigned char size;
	entry_t buffer[DATASTORE_MAX_SIZE];
} fifo_queue_t;

void initialize_fifo(fifo_queue_t *fifo);
unsigned char is_empty(fifo_queue_t *fifo);
void store_data(fifo_queue_t *fifo, unsigned char identifier, long new_data);
unsigned char is_empty(fifo_queue_t *fifo);
unsigned char not_empty(fifo_queue_t *fifo);
void retrieve_data(fifo_queue_t *fifo, entry_t *entry);

/* functions signatures */
void initialize(void);
void process_state(void);
void show_tick(void);
void tick(void);


#endif /* TXRXLPF_H_ */
