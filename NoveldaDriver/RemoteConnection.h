/*
 * EchoCareUnit.h
 *
 *  Created on: 29 Apr 2019
 *      Author: user
 */

#ifndef ECHOCAREUNIT_H_
#define ECHOCAREUNIT_H_

#define MAX_SIZE_OF_PACKETS    1600

extern float image[MAX_SIZE_OF_PACKETS][390]  ;
extern bool packetIsReady ;
extern bool isConnectionEstablished ;
extern bool sendingNow;
extern pthread_mutex_t SendFileMutex ;

extern struct timeval tpStart , tpStop ;
extern struct timeval tp ;
extern float f1 ;


#define BUFF_SIZE     1024
#define SEND_BUF_SIZE2  4*390


typedef enum
{
	ERR = -1 ,
	OK = 0
}res;

typedef struct PROCESS_CHAR
{
	int fd[2] ;
	pid_t pid ;
	char buffer[BUFF_SIZE];
}ProcessCharacters;



typedef struct __attribute__((packed))
{
	char cmd ;
	union {
		float rstart ;
		uint32_t Rstart ;
	}range;
	char baselineNo ;
}remoteMsg;


typedef struct __attribute__((packed))
{
	uint8_t status ;
	uint16_t bins ;
}ackMsg;

int ImxBoard_Sendbuffer( char* buffer , int size );
void* SendFileToRemote( void* args );
void* GetCommandFromRemote( void* args );
bool isconnected();
void closeSockets();

#endif /* ECHOCAREUNIT_H_ */
