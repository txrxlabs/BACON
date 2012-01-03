#include <msp430fg4619.h>
#include "powermgmt.h"
#include "txrxlpf.h"
unsigned char srvHold = 0;
unsigned char sysHold = 0;

volatile unsigned char battState = 0;
void setPwrBusStatus(unsigned char bus, unsigned char state) {
	unsigned char *count;
	switch(bus) {
	case PM_SYSPWR:
		count = & sysHold;
		break;
	case PM_SRVPWR:
		count = & srvHold;
		break;
	}
	if(!state ) {
	   if(!(*count) ) return;
	   *count = (*count)-1;
	   if((*count)>0) return;
	} else *count = (*count)+1;
	if((bus&P8OUT)==state) return;
	else {

		P8OUT = (P8OUT&(~bus)) + (bus&state);
		// todo: add power startup delay here
	}

}
void measureBatt() {
	ADC12CTL0 = SHT0_2 + ADC12ON;             // Sampling time, ADC12 on
	ADC12CTL1 = SHP;                          // Use sampling timer
	ADC12IE = 0x01;                           // Enable interrupt
	ADC12CTL0 |= ENC;
	P6SEL |= 0x01;                            // P6.0 ADC option select
   ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion
	__bis_SR_register(LPM0_bits + GIE);     // LPM0, ADC12_ISR will force exit
}
#pragma vector = ADC12_VECTOR
	__interrupt void ADC12_ISR(void)
	{

	unsigned int batVal = 0;
	batVal = ADC12MEM0;
	__no_operation();
	 if(ADC12MEM0> 4096) battState =2;
	 else if(ADC12MEM0< 1000) battState =1;
	 else battState = 0;
	  __bic_SR_register_on_exit(LPM0_bits);

}
unsigned char manageBatt(){
	// disable solar charging
	// pause to let system settle

	__no_operation();
	measureBatt();// take voltage reading of cell
	__no_operation();
	switch(battState) {
	case 1:	// renable solar cell
		return BATT_LOW;// if cell is low return LOW_CELL
	case 2:
		return BATT_FULL;
	default:
		return BATT_CHRGNG; // if we are charging
	}
}
