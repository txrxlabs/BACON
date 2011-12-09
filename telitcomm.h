/*
 * telitcomm.h
 *
 *  Created on: Nov 9, 2011
 *      Author: hackerspace
 */

#ifndef TELITCOMM_H_
#define TELITCOMM_H_

#define HOSTNAME "50.17.231.113"
#define MAX_CONN_ATTEMPTS = 2

typedef enum {
	STATE_UNINIT = 0,
    STATE_INIT,
    STATE_OPEN,
    STATE_ERROR
} gprs_state_t;

typedef unsigned int (*p2callback)(void);

typedef struct {
	char **cmd;
	p2callback callback;
} telitcmd;

unsigned char isRXBufferEmpty (void);
unsigned char getRXBufferChar (void);
void clearRXBuffer();
void do_init();
void do_uninit();
unsigned int do_reset();
unsigned int do_open();
unsigned int do_close();
unsigned int gprsSend(unsigned char *data, unsigned int len);
unsigned int gprsClose();
unsigned int ackOK(void);
unsigned int ackSGACT(void);
unsigned int ackSend(void);
unsigned int ackSD(void);
unsigned int ackNoOp(void);
unsigned int sendData(unsigned char *data, unsigned int len);
unsigned int sendCmd(telitcmd tcmd);
unsigned char getResponse();

#endif /* TELITCOMM_H_ */
