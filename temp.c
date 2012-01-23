//******************************************************************************
//
//
//   R. von Kurnatowski III
//   TX/RX Labs
//   November 2011
//   Built with CCE Version: 4.0.1
//******************************************************************************
#include "txrxlpf.h"
#include "msp430xG46x.h"
#include "temp.h"
#include "powermgmt.h"





void initTemp() {

	CACTL1 =  CAON + CAREF_1+CARSEL;             // Enable Comp, ref = 0.5*Vcc
	CACTL2 = P2CA0 + CAF;                           // Pin to CA0

	TACCTL1 |= CM_2 + CCIS_1 + CAP;

}
// note all pins connnected to cap not included as d and c must be high impedence
unsigned int measureDis(unsigned char cpin, unsigned char dpin) {
			unsigned int dtime;
			__no_operation();
			P8SEL &= ~cpin;
			P8SEL &= ~dpin;
			P8DIR &= ~dpin;
			P8DIR |= cpin;//charge cap using cpin
			P8OUT |= cpin;
			P8OUT |= dpin;

			TACCTL1 |= CCIE;
			TACTL =  TASSEL_1;
			TACTL |= TACLR;
			CACTL1 =  CAON;
			__no_operation();
			P8DIR &= ~cpin; //high impedence cpin
			P8DIR |= dpin; // dpin output and high

			TACTL |= MC_2; // start capture timer counting

			P8OUT &= ~dpin; // start discharge using dpin
			LPM3;// wait for capture interrupt in lpm
			TACCTL1 &= ~CCIE;
			dtime = TACCR1;// get time value
			P8DIR &= ~dpin; // high impendence dpin
			TACTL = MC_0;
			CACTL1 &=  ~CAON;
			return dtime;

}


void executeTemp(unsigned char* buffer, unsigned int length) {
	initTemp();
	unsigned int a = measureDis(BIT4, BIT4);
	unsigned int b = measureDis(BIT4, BIT5);
	__no_operation();
	*buffer = (unsigned char)((a & 0xFF00)>>8);
	buffer++;
	*buffer = (unsigned char)((a & 0x00FF));
	buffer++;
	*buffer = (unsigned char)((b & 0xFF00)>>8);
	buffer++;
	*buffer = (unsigned char)((b & 0x00FF));
	buffer++;
	return;
}

// Timer_A3 Interrupt Vector (TAIV) handler
#pragma vector=TIMERA1_VECTOR
__interrupt void Timer_A1(void)
{
  switch( TAIV )
  {
  case  2:
	  	  __no_operation();
	  	__bic_SR_register_on_exit(LPM3_bits);

           break;
  default:
	  __no_operation();
	  __bic_SR_register_on_exit(LPM3_bits);
	  break;
 }
}






