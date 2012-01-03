//******************************************************************************
//   Telit Communications Manager for Project Bacon
//
//   Description: Initializes Telit GE865-QUAD and posts datapacket to server
//   USCI_A0 RX interrupt triggers fills circular buffer
//   ACLK = BRCLK = LFXT1 = 32768, MCLK = SMCLK = DCO~1048k
//   //* An external watch crystal is required on XIN XOUT for ACLK *//
//
//            |     P4.7/UCA0RXD|------------>
//            |                 | 4800 - 8N1
//            |     P4.6/UCA0TXD|<------------
//
//   R. von Kurnatowski III
//   TX/RX Labs
//   November 2011
//   Built with CCE Version: 4.0.1
//******************************************************************************

#include  "msp430xg46x.h"
#include "telitcomm.h"
#include "txrxlpf.h"
#include "powermgmt.h"
#include "intrinsics.h"
#include "in430.h"
#include <stdio.h>
/*  Global Variables  */
static char resp[40];
volatile unsigned char ucRXBuffer[32];
volatile unsigned char ucWriteIndex = 0;
volatile unsigned char ucReadIndex = 0;
extern volatile unsigned char timedout;
gprs_state_t gprs_state = STATE_UNINIT;

static char sgactt1[] = "#";
static char sgactt2[] = "\n0";
static char *atstr[]           = {"AT\r",0};
static telitcmd at             = {atstr,&ackOK};

static char *atechostr[]       = {"ATE\r",0};
static telitcmd atecho         = {atechostr, &ackOK};

static char *atvstr[]          = {"ATV0\r",0};
static telitcmd atv            = {atvstr, &ackOK};

static char *atqstr[]          = {"ATQ0\r",0};
static telitcmd atq            = {atqstr, &ackOK};

static char *atflowctlstr[]    = {"AT&K=0\r",0};
static telitcmd atflowctl      = {atflowctlstr, &ackOK};

static char *atsetusidstr[]    = {"AT#USERID=\"WAP@CINGULARGPRS.COM\"\r",0};
static telitcmd atsetusid      = {atsetusidstr, &ackOK};

static char *atsetpassstr[]    = {"AT#PASSW=\"CINGULAR1\"\r",0};
static telitcmd atsetpass      = {atsetpassstr, &ackOK};

static char *atcgdcontstr[]    = {"AT+CGDCONT=1,\"IP\",\"WAP.CINGULAR\"\r", 0};
static telitcmd atcgdcont      = {atcgdcontstr, &ackOK};

static char *atstorestr[]      = {"AT&W\r", 0};
static telitcmd atstore        = {atstorestr, &ackOK};

static char *atconnectstr[]    = {"AT#SGACT=1,1\r",0};
static telitcmd atconnect      = {atconnectstr, &ackSGACT};

static char hostname[]         = HOSTNAME;
static char hostport[]         = HOSTPORT;
static char *atopenscktstr[]   = {"AT#SD=1,0,",hostport,",\"",hostname,"\"\r",0};
static telitcmd atopensckt     = {atopenscktstr, &ackSD};

static char postpath[]         = POSTPATH;
static char requesthost[]      = REQUESTHOST;
static char preamble[]         = PREAMBLE;
static char moduleid[]         = MODULEID;
static char *atpoststr[]       = {"POST ", postpath,  " HTTP/1.1\r\nhost: ", requesthost,
								  "\r\nTransfer-Encoding: chunked\r\n\r\n8\r\n", preamble,
								  moduleid, "\r\n", 0};
static telitcmd atpost         = {atpoststr, &ackNoOp};

static char *atposttermstr[]   = {"4\r\n\xFF\xFF\xFF\xFF\r\n0\r\n\r\n", 0};
static telitcmd atpostterm     = {atposttermstr, &ackSend};

static char *atdisconnectstr[] = {"AT#SGACT=1,0\r",0};
static telitcmd atdisconnect   = {atdisconnectstr, &ackOK};

static char *atclosestr[]      = {"AT#SH=1\r",0};
static telitcmd atclose        = {atclosestr, &ackOK};

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIA0RX_ISR (void)
{
	ucRXBuffer[ucWriteIndex++] = UCA0RXBUF; // store received byte and
	ucWriteIndex &= 0x1f; // reset index

}

unsigned char isRXBufferEmpty (void) {
	return (ucWriteIndex == ucReadIndex);
}

unsigned char getRXBufferChar (void) {
	unsigned char Byte;
	if (ucWriteIndex != ucReadIndex) { // char still available
		Byte = ucRXBuffer[ucReadIndex++]; // get byte from buffer
		ucReadIndex &= 0x1f; // adjust index
		return (Byte);
	} else return (0); // if there is no new char
}

void clearRXBuffer() {
	IE2 &= ~UCA0RXIE;                          // Disable USCI_A0 RX interrupt
	ucReadIndex=0;
	ucWriteIndex = 0;
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}

void do_init() {
	P4SEL |= 0x0C0;

	// P4.7,6 = USCI_A0 RXD/TXD
	UCA0CTL1 |= UCSWRST;
	UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
	UCA0BR0 = 0x06;                           // 32k/9600 - 3.41
	UCA0BR1 = 0x00;                           //
	UCA0MCTL = UCBRS2 +UCBRS1+ UCBRS0;                          // Modulation
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	gprs_state = STATE_INIT;
}

void do_uninit() {
	UCA0CTL1 |= UCSWRST;
	gprs_state = STATE_UNINIT;
}
unsigned int do_power() {
		setPwrBusStatus(PM_SYSPWR, PM_ENABLE);
		__no_operation();
		P8SEL &= 0x00;
		P8OUT |= 0x02;
		P8DIR |= 0x02;
		P8DIR &= ~0x01;

		// make sure uart pins are all low
		P4SEL &= 0x3F;  //00111111
		P4OUT &= 0x3F;  //00111111
		P4DIR |= 0xC0; //11000000

	if(!(P8IN & 0x01)) {
	     P8OUT &= 0xFD;
	     setTimeout(2);
	     __bis_SR_register(LPM3);
	     __no_operation();
	     P8OUT |= 0x02;
	     setTimeout(2);
	     __bis_SR_register(LPM3);
	     __no_operation();
	     if(!(P8IN & 0x01)) {
	    	 __no_operation();
	    	 return 1;
	     }
	}
	__no_operation();
	gprs_state = STATE_PWRD;
	return 0;
}

unsigned int do_unpower() {
	        P8OUT |= 0x02;
			P8DIR |= 0x02;
			P8DIR &= 0xFE;
			// make sure uart pins are all low
			P4SEL &= 0x3F;  //00111111
			P4OUT &= 0x3F;  //00111111
			P4DIR |= 0xC0; //11000000

		if((P8IN & 0x01)) {
		     P8OUT &= 0xFD;

		     setTimeout(2);
		    	     __bis_SR_register(LPM3);
		    	     __no_operation();
		    	     P8OUT |= 0x02;
		    	     setTimeout(2);
		    	     __bis_SR_register(LPM3);
		    	     __no_operation();
		     if((P8IN & 0x01)) return 1;
		}

		setPwrBusStatus(PM_SYSPWR, PM_DISABLE);
	gprs_state = STATE_UNINIT;
	return 0;

}
unsigned int do_reset() {

	telitcmd *cmds[10] = {&atecho, &atv, &atq, &atflowctl, &atsetusid, &atsetpass, &atcgdcont, &atstore, &atclose, &atdisconnect};
	unsigned char pos = 0;
	while(pos<10) sendCmd(*(cmds[pos++]));
	gprs_state = STATE_INIT;
	return 0;
}
unsigned int do_open() {
	telitcmd *cmds[4] = {&at, &atconnect, &atopensckt, &atpost};
	unsigned char pos = 0;
	unsigned char retried = 0;
	while(pos<4) {
		if(sendCmd(*(cmds[pos++]))) {
			if(retried) {
				return 1;
			} else {
				do_reset();
				pos=0;
				retried = 1;
			}
		}
	}
	gprs_state = STATE_OPEN;
	return 0;
}

unsigned int do_close() {
	telitcmd *cmds[3] = {&atpostterm, &atdisconnect, &atclose};
	unsigned char pos = 0;
	while(pos<3) sendCmd(*(cmds[pos++]));
	gprs_state = STATE_INIT;
	return 0;
}

unsigned int gprsSend(unsigned char *data, unsigned int len) {
	switch (gprs_state) {
	case STATE_UNINIT:
		if(do_power()) {
			__no_operation();
			return 1;
		}
	case STATE_PWRD:
		do_init();
	case STATE_INIT:
		if(do_open()) {
			__no_operation();
			return 1;
		}
	default:
		break;
	}

	return sendData(data, len);
}

unsigned int gprsClose() {

	switch (gprs_state) {
	case STATE_OPEN:
	case STATE_ERROR:
		do_close();
	case STATE_INIT:
		do_uninit();
	case STATE_PWRD:
		do_unpower();
	case STATE_UNINIT:
	default:
		return 0;
	}
}

unsigned int memcmp(char *a, char *b, unsigned int length) {
	while(length>0) {
		if(*a != *b) return 1;
		length--;
		a++;
		b++;
	}
	return 0;
}
unsigned int ackOK(void) {
	getResponse();
	return memcmp(resp, "0", 2);
}
unsigned int ackSGACT(void) {
	getResponse();

	if(memcmp(resp, sgactt1, 1)) {
		__no_operation();
		return 1;
	}
	getResponse();
	if(memcmp(resp, sgactt2, 2)) {
		__no_operation();
		return 1;
	}
	__no_operation();
	return 0;
}

unsigned int ackSend(void) {
	unsigned int isbody = 1;
	while(isbody) {
		getResponse();
		isbody = !(memcmp(resp, "0", 2));
	}
	return 0;
}
unsigned int ackSD(void) {
	getResponse();
	return memcmp(resp, "1", 2);
}

unsigned int ackNoOp(void) {
	return 0;
}
unsigned int sendData(unsigned char *data, unsigned int len) {
	unsigned int pos = 0;
	char lb[7];
	sprintf(lb, "%x\r\n", (int)len);// get length in hex


	while(lb[pos] != '\0') {
		while(!(IFG2&UCA0TXIFG));
		UCA0TXBUF = lb[pos++];
	}
	pos =0;
	while(pos < len) {
		while(!(IFG2&UCA0TXIFG));
		UCA0TXBUF = data[pos];
		pos++;
	}
	pos =0;
	char eb[] = "\r\n";
	while(eb[pos] != '\0') {
		while(!(IFG2&UCA0TXIFG));
		UCA0TXBUF = eb[pos];
		pos++;
	}

	return 0;
}

unsigned int sendCmd(telitcmd tcmd) {

	unsigned int ret;
	unsigned int pos=0;
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
	clearRXBuffer();
	while(*(tcmd.cmd+pos) != 0) {
		char *c = *(tcmd.cmd+pos);
		pos++;
		while(*c!='\0') {
			while(!(IFG2&UCA0TXIFG));
			UCA0TXBUF = *c;
			c++;
		}
	}
	ret = tcmd.callback();
	IE2 &= ~UCA0RXIE;                          // Enable USCI_A0 RX interrupt
	return ret;

}
unsigned char getResponse() {
	unsigned char ptr = 0;
	setTimeout(5);
	unsigned char c;
	while(!timedout) {
		if (!isRXBufferEmpty()) {
			c = getRXBufferChar();
			if(c=='\r' || ptr>38) {
				*(resp+ptr) = '\0';
				clearTimeout();
				return 0;
			} else *(resp+(ptr++)) = c;
		}
	}
	clearTimeout();
	return 1;
}
