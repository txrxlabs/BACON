/* TX/RX Low Power Framework
 */

//#include <msp430g2211.h>
#include <msp430fg4619.h>
#include "txrxlpf.h"
#include "telitcomm.h"
#include "powermgmt.h"


// unsigned char seconds_counter=0; /* used for lpm1 */
volatile unsigned int second_intervals=0;
volatile unsigned char timermode = 0;
volatile unsigned int timeout = 0;
volatile unsigned char timedout = 0;
volatile unsigned int timeout_intervals;
//
unsigned char dataBuffer[TRANSMIT_BUFFER_SIZE];
unsigned int dataPntr  = 0;
irqVector vectorMap[2][8];

extern procFcn procMap[];
control_state_t control_state=STATE_INITIAL;


void enableSensors() {

}

void attachInterrupt(irqVector vector, unsigned char port, unsigned char bit, unsigned char edge) {
	unsigned char offset = port==1?8:0;
	char *pdir = (char*)(0x022 + offset);
	char *psel = (char*)(0x026 + offset);
	char *pie  = (char*)(0x025 + offset);
	char *pies = (char*)(0x024 + offset);
	*pdir &= ~(0x01 << bit);
	*psel &= ~(0x01 << bit);
	if(!edge) *pies &= ~(0x01 << bit);
	else *pies |= (0x01 << bit);
	*pie |= (0x01 << bit);
	vectorMap[port][bit] = vector;
}
void detachInterrupt(unsigned char port, unsigned char bit) {
	unsigned char offset = port==1?8:0;
	char *pie = (char*)(0x025 + offset);
	*pie &= ~(0x01 << bit);
	vectorMap[port][bit] = 0;
}
unsigned char* getBuffer(unsigned int length) {
	if(length >TRANSMIT_BUFFER_SIZE) return 0;
	if(dataPntr+length <= TRANSMIT_BUFFER_SIZE) {
		unsigned char *retptr = dataBuffer+dataPntr;
		dataPntr+=length;
		return retptr;
	} else {
		flushTransmission();
		return getBuffer(length);
	}
}

unsigned int releaseBuffer(unsigned int length) {
	if (length > dataPntr) {
		dataPntr = 0;
		return 1;
	}
	dataPntr -= length;
	return 0;
}
void logError() {
	unsigned char *buf = getBuffer(10);
	*(buf++) = 0xEE;
	*(buf++) = 0xEE;
	*(buf++) = 0xEE;
	*(buf++) = 0xEE;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
}
void logBattVal(unsigned char battVal) {
	unsigned char *buf = getBuffer(12);
	*(buf++) = 0xBB;
	*(buf++) = 0xBB;
	*(buf++) = 0xBB;
	*(buf++) = 0xBB;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x00;
	*(buf++) = 0x02;
	*(buf++) = 0xF0&battVal;
	*(buf++) = 0x0F&battVal;
}

void flushTransmission() {
	if(gprsSend(dataBuffer, dataPntr)) {
		dataPntr=0;
		logError();
	} else {
		dataPntr=0;
	}
	return ;
}

void closeTransmission() {
flushTransmission();
gprsClose();
}





void executeProcs(void) {
	unsigned char proc =0;
	proc_struct_t proc_struct = {0x00};
	while(procMap[proc]!=0) {
		procMap[proc](proc_struct);
		proc++;
	}
	closeTransmission();
}



void configure_timer(void) {
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

		P5DIR |= (BIT7 + BIT6 + BIT5 + BIT4); // setup diag lights
		P5OUT |= (BIT7 + BIT6 + BIT5 + BIT4); // setup diag lights
		// set all pins to inital low power modes
		P6SEL |= 0x01;                            // P6.0 ADC option select
		// Setup GPIO pins for power and gsm modem management
		P8SEL &= 0x00;
		P8OUT |= 0x02;
		P8DIR |= 0x02;
		P8DIR &= ~0x01;

		unsigned int wait =0;
		while(wait<3000) {
			wait++;
			__no_operation();
		}
	configure_timer();
	_EINT();
	P5OUT &= 0x0F;
}

void process_state(void)
{
	unsigned char battVal;
    switch(control_state)
    {
        case STATE_INITIAL:
        	control_state = STATE_NORMAL;
        	break;
        case STATE_NORMAL:
        	battVal = manageBatt();
        	if( battVal == BATT_LOW) {
        		control_state = STATE_BATTERY_LOW;
        		return;
        	}
        	logBattVal(battVal);
        	executeProcs(); // process data calls
        	break;
        case STATE_BATTERY_LOW:
        	battVal = manageBatt();
        	if( battVal == 0) {
        		control_state = STATE_BATTERY_LOW;
        	    return;
           	}
        	control_state= STATE_NORMAL;
        	logError();
        	logBattVal(battVal);
        	executeProcs(); // process data calls
            break;
    }
}

void setTimeout(unsigned int time) {
	timeout = time;
	timedout = 0;
	timeout_intervals=0;
	timermode |= 0x02;
	IE2 |= BTIE;
	P5OUT |= 0x10;
	P5OUT &= ~(0x20);
}
void clearTimeout() {
	timermode &= 0xFD;
	if(!timermode) IE2 &= ~BTIE;
	timedout = 0;
	P5OUT &= ~(0x30);
}

void enablePeriodic() {
	second_intervals = 0;
	timermode |= 0x01;
	IE2 |= BTIE;

}

void disablePeriodic() {
	timermode &= 0xFE;
	if(!timermode) IE2 &= ~BTIE;
}

void main(void) {
	initialize();
	while(1) {
		disablePeriodic();
		P5OUT |= 0x40;
		process_state();
		P5OUT &= ~(0x40);
		enablePeriodic();
		__bis_SR_register(LPM3_bits + GIE);
	}
} // main


//should probably set the
// LPM3 Timer interrupt
#pragma vector=BASICTIMER_VECTOR
__interrupt void basic_timer(void) {
	second_intervals++;
	timeout_intervals++;
	P5OUT ^= 0x80;
		if ((timermode & 0x01) && (second_intervals == PULSE_LENGTH)) {
			control_state = STATE_NORMAL;
			second_intervals=0;
			__bic_SR_register_on_exit(LPM3_bits);
		}
		if ((timermode & 0x02) && (timeout_intervals == timeout)) {
					timeout_intervals=0;
					timermode &= 0xFD;
					timedout = 1;
					P5OUT |= 0x20;
					P5OUT &= ~(0x10);
					__bic_SR_register_on_exit(LPM3_bits);
		}


}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
	unsigned char ipos = 0;
	for(;ipos<8;ipos++) {
	if(P1IFG & 0x01<< ipos) {
		P1IFG &= ~0x01<<ipos;
		__bic_SR_register_on_exit(vectorMap[0][ipos]());
	}
	}
}
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void) {
	unsigned char ipos = 0;
		for(;ipos<8;ipos++) {
		if(P2IFG & 0x01<< ipos) {
			P2IFG &= ~0x01<<ipos;
			__bic_SR_register_on_exit(vectorMap[1][ipos]());
		}
		}
}


