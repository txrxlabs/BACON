/* TX/RX Low Power Framework
 */

//#include <msp430g2211.h>
#include <msp430fg4619.h>
#include <stdlib.h>
#include <stdio.h>
#include "txrxlpf.h"
#include "telitcomm.h"


// unsigned char seconds_counter=0; /* used for lpm1 */
int two_second_intervals=0;
int minutes_counter=0;

control_state_t control_state=STATE_INITIAL;

fifo_queue_t datastore;
fifo_queue_t transmit_log;

unsigned int tx_byte;

/* for temp measurement stub */
long tempAverage;

// For debug purposes
void show_tick(void) {
	P1OUT ^= RLY1;
}

void tick(void) {
	two_second_intervals++;
	if (two_second_intervals == MINUTE_LENGTH) {
		minutes_counter++;
		two_second_intervals=0;
	}

	if (minutes_counter == PULSE_LENGTH) {
		show_tick();
		process_state();
		minutes_counter=0;
	}
}

/*
// Half second timer for LPM1
void tick(void) {
	static char half_seconds=0;
	if (++half_seconds == 1) {
		seconds_counter++;
	} else {
		half_seconds=0;
	}
	if (seconds_counter == MINUTE_LENGTH) {
		minutes_counter++;
		seconds_counter=0;
	}
	if (minutes_counter == PULSE_LENGTH) {
		show_tick();
		process_state();
		minutes_counter=0;
	}
}
*/

void measure_temperature(void) {
	//char i;
    /* Moving average filter out of 8 values to somewhat stabilize sampled ADC */
    //tempAverage = 0;
	//for (i = 0; i < 8; i++)
    //	tempAverage += ADC10MEM;
    tempAverage = 15;                      // Divide by 8 to get average

    store_data(&datastore, (unsigned char)THERMOMETER_ID, tempAverage);
}

// Function Transmits Character from TXByte
void Transmit(void) {
	entry_t item;
	while(not_empty(&datastore)) {
		retrieve_data(&datastore, &item);

		// This is where you'd do the transmitting
		// store_data(&transmit_log, item.instrument_id, item.data);
		IE2 &= ~BTIE;
		 transComms();
		 IE2 |= BTIE;
	}
}

void configure_timer(void) {
	//BCSCTL1 = CALBC1_1MHZ;    // Set DCO to calibrated 1 MHz.
	//DCOCTL = CALDCO_1MHZ;

	/*
	// Section for LPM1
	TACCR0 = 62500 - 1;    // A period of 62,500 cycles is 0 to 62,499.
	TACCTL0 = CCIE;        // Enable interrupts for CCR0.
	TACTL = TASSEL_2 + ID_3 + MC_1 + TACLR;  // SMCLK, div 8, up mode,
	                                         // clear timer
	*/

	// Section for LPM3
	IE2 |= BTIE;                              // Enable BT interrupt
	BTCTL = BTDIV+BTIP2+BTIP1+BTIP0;          // 2s Interrupt
}

void initialize(void) {
	volatile unsigned int i;
	WDTCTL = WDTPW + WDTHOLD;    // disable watchdog
	FLL_CTL0 |= XCAP14PF;                     // Configure load caps

		do
		{
			IFG1 &= ~OFIFG;                           // Clear OSCFault flag
			for (i = 0x47FF; i > 0; i--);             // Time for flag to set
		}
		while ((IFG1 & OFIFG));                   // OSCFault flag still set?

		initComms();
			P1OUT = 0;
	P1DIR = RLY1;    // P1.6 out to relay (LED)

	configure_timer();

	initialize_fifo(&datastore);
	initialize_fifo(&transmit_log);

	// Initialize Instruments

    // ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start, internal thermometer

	//_enable_interrupt();
	_BIS_SR(LPM3_bits + GIE);	// Enter LPM3 and enable interrupts
}

void process_state(void)
{
    switch(control_state)
    {
        case STATE_INITIAL:
        	control_state = STATE_NORMAL;
        	break;
        case STATE_NORMAL:
        	measure_temperature();
        	Transmit();
            break;
        case STATE_BATTERY_LOW:

            break;
    }
}

void initialize_fifo(fifo_queue_t *fifo) {
	unsigned char i;
	fifo->front = 0x00;
	fifo->rear = 0x00;
	fifo->size = 0x00;
	for(i = 0; i < DATASTORE_MAX_SIZE; i++) {
		((fifo->buffer)[i]).instrument_id = 0x00;
		((fifo->buffer)[i]).data = 0x0000;
	}
}

void store_data(fifo_queue_t *fifo, unsigned char identifier, long new_data) {
	((fifo->buffer)[fifo->front]).instrument_id = identifier;
	((fifo->buffer)[fifo->front]).data = new_data;
	(fifo->front) += 1;
	if (fifo->front >= DATASTORE_MAX_SIZE) {
		fifo->front = 0;
	}
	if ((fifo->size) < DATASTORE_MAX_SIZE) {
		(fifo->size)++;
	}
	else if ((fifo->size) >= DATASTORE_MAX_SIZE) {
		(fifo->rear)++;
		if (fifo->rear >= DATASTORE_MAX_SIZE) {
			fifo->rear = 0;
		}
	}
}

unsigned char is_empty(fifo_queue_t *fifo) {
	return (fifo->size) == 0;
}

unsigned char not_empty(fifo_queue_t *fifo) {
	return (fifo->size) > 0;
}

void retrieve_data(fifo_queue_t *fifo, entry_t *entry) {
	if (fifo->size > 0) {
		entry->instrument_id = ((fifo->buffer)[fifo->rear]).instrument_id;
		entry->data = ((fifo->buffer)[fifo->rear]).data;
		(fifo->rear)++;
		if (fifo->rear >= DATASTORE_MAX_SIZE) {
			fifo->rear = 0;
		}
		(fifo->size)--;
	}
}

void main(void) {
	initialize();
	for(;;) {
	}
} // main


// Interrupt Service Routines

/*
//LPM1 service routine
#pragma vector = TIMERA0_VECTOR
__interrupt void CCR0_ISR(void) {
	tick();
} // CCR0_ISR
*/

// LPM3 Timer interrupt
#pragma vector=BASICTIMER_VECTOR
__interrupt void basic_timer(void) {
	tick();
}

