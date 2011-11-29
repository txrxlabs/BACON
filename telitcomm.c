//******************************************************************************
//   MSP430xG46x Demo - USCI_A0, Ultra-Low Pwr UART 9600 Echo ISR, 32kHz ACLK
//
//   Description: Echo a received character, RX ISR used. Normal mode is LPM3,
//   USCI_A0 RX interrupt triggers TX Echo.
//   ACLK = BRCLK = LFXT1 = 32768, MCLK = SMCLK = DCO~1048k
//   Baud rate divider with 32768hz XTAL @9600 = 32768Hz/9600 = 3.41 (0003h 03h )
//   //* An external watch crystal is required on XIN XOUT for ACLK *//
//
//
//                MSP430xG461x
//             -----------------
//         /|\|              XIN|-
//          | |                 | 32kHz
//          --|RST          XOUT|-
//            |                 |
//            |     P4.7/UCA0RXD|------------>
//            |                 | 9600 - 8N1
//            |     P4.6/UCA0TXD|<------------
//
//   K. Quiring/ M. Mitchell
//   Texas Instruments Inc.
//   October 2006
//   Built with CCE Version: 3.2.0 and IAR Embedded Workbench Version: 3.41A
//******************************************************************************
#include  "msp430xG46x.h"
#include <stdlib.h>
#include <stdio.h>
#include <String.h>
#include "telitcomm.h"

/*  Global Variables  */
char mbi[128];
unsigned int mbpos = 0;
char ucRXBuffer[32];
unsigned char ucWriteIndex = 0;
unsigned char ucReadIndex = 0;
volatile unsigned int cmdstatus;
volatile unsigned int i;
volatile unsigned int ADCresult;
volatile unsigned long int DegC, DegF;
volatile unsigned char timedout;
static char sgactt1[] = "#";
static char sgactt2[] = "\n0";
static char *atstr[]           = {"AT\r",NULL};
static telitcmd at           = {atstr,&ackOK};
static char *atechostr[]       = {"ATE\r",NULL};
static telitcmd atecho       = {atechostr, &ackOK};
static char *atvstr[]       = {"ATV0\r",NULL};
static telitcmd atv       = {atvstr, &ackOK};
static char *atqstr[]       = {"ATQ0\r",NULL};
static telitcmd atq        = {atqstr, &ackOK};
static char *atflowctlstr[]    = {"AT&K=0\r",NULL};
static telitcmd atflowctl    = {atflowctlstr, &ackOK};
static char *atsetusidstr[]    = {"AT#USERID=\"WAP@CINGULARGPRS.COM\"\r",NULL};
static telitcmd atsetusid    = {atsetusidstr, &ackOK};
static char *atsetpassstr[]    = {"AT#PASSW=\"CINGULAR1\"\r",NULL};
static telitcmd atsetpass    = {atsetpassstr, &ackOK};
static char *atconnectstr[]    = {"AT#SGACT=1,1\r",NULL};
static telitcmd atconnect    = {atconnectstr, &ackSGACT};
static char *atopenscktstr[]   = {"AT#SD=1,0,3000,\"","50.17.231.113","\"\r",NULL};
static telitcmd atopensckt   = {atopenscktstr, &ackSD};
static char *atdisconnectstr[] = {"AT#SGACT=1,0\r",NULL};
static telitcmd atdisconnect = {atdisconnectstr, &ackOK};
static char *atcgdcontstr[] = {"AT+CGDCONT=1,\"IP\",\"WAP.CINGULAR\"\r", NULL};
static telitcmd atcgdcont = {atcgdcontstr, &ackOK};
static char *atclosestr[] = {"AT#SH=1\r",NULL};
static telitcmd atclose = {atclosestr, &ackOK};
static char postterm[] = {(char)0x1a,'+','+','+','\0'};
static char *atpoststr[]       = {"POST /posttest HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ", NULL, "\r\nHost: ", "50.17.231.113", "\r\n\r\n", "blob=", NULL, postterm , NULL};
static telitcmd atpost         = {atpoststr, &ackPost};

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR(void)
{
  ADCresult = ADC12MEM0;                    // Move results, IFG is cleared
  __bic_SR_register_on_exit(LPM0_bits);     // Exit LPM0
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIA0RX_ISR (void)
{

	//_EINT();
	mbi[mbpos++] = UCA0RXBUF;
	ucRXBuffer[ucWriteIndex++] = UCA0RXBUF; // store received byte and
	// inc receive index
	ucWriteIndex &= 0x1f; // reset index

}
char isRXBufferEmpty (void) {
	return (ucWriteIndex == ucReadIndex);
}

char getRXBufferChar (void) {
	char Byte;
	if (ucWriteIndex != ucReadIndex) { // char still available
		//IE2 &= ~UCA0RXIE;                          // Disable USCI_A0 RX interrupt
		Byte = ucRXBuffer[ucReadIndex++]; // get byte from buffer
		ucReadIndex &= 0x1f; // adjust index
		//IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
		return (Byte);
	} else return (0); // if there is no new char
}

void clearRXBuffer() {
	IE2 &= ~UCA0RXIE;                          // Disable USCI_A0 RX interrupt
	ucReadIndex=0; // get byte from buffer
	ucWriteIndex = 0; // adjust index
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}

void initComms() {
	P4SEL |= 0x0C0;                           // P4.7,6 = USCI_A0 RXD/TXD
	  UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
	  UCA0BR0 = 0x06;                           // 32k/9600 - 3.41
	  UCA0BR1 = 0x00;                           //
	  UCA0MCTL = UCBRS2 +UCBRS1+ UCBRS0;                          // Modulation
	  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	  IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

	   ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_6;
	                                             // Setup ADC12, ref., sampling time
	   ADC12CTL1 = SHP;                          // Use sampling timer
	   ADC12MCTL0 = INCH_10 + SREF_1;            // Select channel A10, Vref+
	   ADC12IE = 0x01;                           // Enable ADC12IFG.0
	   for (i = 0; i < 0x3600; i++);             // Delay for reference start-up
	   ADC12CTL0 |= ENC;                         // Enable conversions


}

void transComms() {

	ADC12CTL0 |= ADC12SC;                   // Start conversion


	__bis_SR_register(LPM0_bits + GIE);           // Enter LPM0

	     //  DegC = (Vsensor - 986mV)/3.55mV
	     //  Vsensor = (Vref)(ADCresult)/4095)
	     //  DegC -> ((ADCresult - 1615)*704)/4095
	     DegC = ((((long)ADCresult-1615)*704)/4095);
	     DegF = ((DegC * 9/5)+32);               // Calculate DegF
	     __no_operation();                       // SET BREAKPOINT HERE

	     char lb[5];
	     sprintf(lb, "%d", DegF );
	    int attempt = 2;
    	int state = 0;
	    while(attempt) {
	    __no_operation();
	    	switch (state) {
	    		case 0:
	    			if(sendCmd(at)) state = 5;
	    			else state = 1;
	    			__no_operation();
	    			break;
	    		case 1:
	    			if(sendCmd(atconnect)) state = 5;
	    			else state = 2;
	    			__no_operation();
	    			break;
	    		case 2:
	    			if(performPost(lb)) state = 5;
	    			else state = 3;
	    			__no_operation();
	    			break;
	    		case 3:
	    			sendCmd(atclose);
	    			sendCmd(atdisconnect);
	    			attempt = 0;
	    			__no_operation();
	    			break;
	    		case 5:
	    			attempt--;
	    			__no_operation();
	    			sendCmd(atecho);
	    			sendCmd(atv);
	    			sendCmd(atq);
	    			sendCmd(atflowctl);
	    			sendCmd(atcgdcont);
	    			sendCmd(atsetusid);
	    			sendCmd(atsetpass);
	    			sendCmd(atclose);
	    			sendCmd(atdisconnect);
	    			state = 0;
	    			break;
		    }
	    }
	    	}




int performPost(char data[]) {
	char lb[7];
	unsigned int datalen = strlen(data);
	unsigned int msglen = datalen+ strlen(*(atpost.cmd+5));
	sprintf(lb, "%d", msglen);
	*(atpost.cmd+1)= lb;
	*(atpost.cmd+6)= data;
	if(sendCmd(atopensckt)) return 1;
	if(sendCmd(atpost)) return 1;
	return 0;
}

int ackOK(void) {
	char *ret = getResponse();
	int val = strcmp(ret, "0");
	free(ret);
	return val;
}
int ackSGACT(void) {
	char *ret1 = getResponse();

	if(memcmp(ret1, sgactt1, 1)) {
		__no_operation();
		free(ret1);
		return 1;
	}
	char *ret2 = getResponse();
	if(strcmp(ret2, sgactt2)) {
		__no_operation();
		free(ret2);
		free(ret1);
		return 1;
	}
	__no_operation();
	free(ret2);
	free(ret1);
	return 0;
}

int ackPost(void) {
	//
	getResponse();
	return 0;
}
int ackSD(void) {
	//
	 getResponse();
	 return 0;
}

int sendCmd(telitcmd tcmd) {
	unsigned int pos=0;
	clearRXBuffer();
	while(*(tcmd.cmd+pos) != NULL) {
	char *c = *(tcmd.cmd+pos);
	pos++;
		while(*c!='\0') {
		while(!(IFG2&UCA0TXIFG));
		UCA0TXBUF = *c;
		c+=1;
	}
	}
	return tcmd.callback();
}
char* getResponse() {
	char *ret = malloc(sizeof(char)*40);
	unsigned char ptr = 0;
	timedout = 0;
	char c;
	while(!timedout) {
		if (!isRXBufferEmpty()) {
			c = getRXBufferChar();
			if(c=='\r' || ptr>38) {
				*(ret+ptr) = '\0';
				return ret;
			} else *(ret+(ptr++)) = c;
		}
	}
	return ret;
}
