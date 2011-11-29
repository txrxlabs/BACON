/*
 * telitcomm.h
 *
 *  Created on: Nov 9, 2011
 *      Author: hackerspace
 */

#ifndef TELITCOMM_H_
#define TELITCOMM_H_


#define HOSTNAME "50.17.231.113";
#define MAX_CONN_ATTEMPTS = 2;

typedef int (*p2callback)(void);

typedef struct {
	char **cmd;
	p2callback callback;
} telitcmd;



char isRXBufferEmpty (void);
char getRXBufferChar (void);
void clearRXBuffer();
void initComms();
void transComms();
int initModem();
int closeModem();
int resetModem();
int performPost(char data[]);
int ackOK(void);
int ackSGACT(void);
int ackPost(void);
int ackSD(void);
int sendCmd(telitcmd tcmd);
char* getResponse();




#endif /* TELITCOMM_H_ */
