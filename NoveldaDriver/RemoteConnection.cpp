//============================================================================
// Name        : Test2.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <pthread.h>
#include <errno.h>
#include "RemoteConnection.h"
#include "taskRadar.h"
#include "NoveldaRadar.h"
#include <chrono>



using Clock = std::chrono::high_resolution_clock;


/*
 *  Prototypes
 */
void* SendPacket( char* buffer , uint32_t bufferSize );

/*
 *  Static Variables
 */
static int 					ImxBoardFd;
// static int 					ClientFd;

static struct sockaddr_in   ImxBoardAddr;
static struct sockaddr_in   TcpClientAddr;
static socklen_t TcpClientLen = sizeof(TcpClientAddr);

sockaddr_in clientAddr;
socklen_t clientAddrSize = sizeof(clientAddr);
int clientSocket;
bool isConnectionEstablished =false;
bool sendingNow = false;

clock_t t0 = 0 ;
clock_t t1 = 0 ;
clock_t t2 = 0 ;



struct timeval tpStart , tpStop ;
struct timeval tp ;
float f1 = 0 ;

char BufferForRemote[204] = { 0 } ;

/*
 *
 */
void RemoteAnalzeRemoteCommand( char *buffer  , int size );



int ImxBoardOpenSocketInit( void )
{
#define IMX_BOARD_SOCKET_PORT_NUM 5044

	printf("init udp imx socket\n");
	// create UDP socket
	ImxBoardFd = socket(AF_INET, SOCK_STREAM, 0);
	if (ImxBoardFd < 0)
	{
		printf("Unable to open socket");
		return -1;
	}

	// initialize radio address structure
	memset(&ImxBoardAddr, 0, sizeof(ImxBoardAddr));
	ImxBoardAddr.sin_family = AF_INET;
	ImxBoardAddr.sin_port = htons(IMX_BOARD_SOCKET_PORT_NUM);
	ImxBoardAddr.sin_addr.s_addr = INADDR_ANY;

	  /*
	   * bind: associate the parent socket with a port
	   */
	if (bind(ImxBoardFd, (struct sockaddr *) &ImxBoardAddr, sizeof(ImxBoardAddr)) < 0)
	{
		printf("ERROR on binding\n");
		close(ImxBoardFd);
		return -1 ;
	}

	
	// Listen for connections
	if (listen(ImxBoardFd, 10) == -1) {
		std::cerr << "Error listening on socket\n";
		close(ImxBoardFd);
		return -1;
	}
	
	// Accept connections
	clientAddrSize = sizeof(clientAddr);
	clientSocket = accept(ImxBoardFd, (sockaddr*)&clientAddr, &clientAddrSize);
	if (clientSocket == -1) {
		std::cerr << "Error accepting connection\n";
		close(ImxBoardFd);
		return -1;
	}
	// const char* message = "Hello, client!";
	// send(clientSocket, message, strlen(message), 0);
	
	// Close sockets
	// close(clientSocket);
	// close(ImxBoardFd);



	isConnectionEstablished = true;
	printf("finish connect \n");
	return OK;

}

void closeSockets(){
	close(clientSocket);
	close(ImxBoardFd);
	printf("socket closed");
}

//_____________________________________________________________________________
//
// ImxBoardGetPacket - read a packet from WIFI using TCP
//
//_____________________________________________________________________________

int ImxBoardGetPacket(void *pkt, unsigned maxSize)
{
	ssize_t nbytes  ;
	// copy packet into buffer
	nbytes = recv(clientSocket, (char *)pkt, maxSize, 0);
	// nbytes = recvfrom(ClientFd, (char *)pkt, maxSize, 0, (struct sockaddr*)&TcpClientAddr, &TcpClientLen);

	// Timeout
	return nbytes;
}

//_____________________________________________________________________________
//
// ImxBoardSendPacket - send a packet to the MRM using UDP
//
//_____________________________________________________________________________

int ImxBoardSendPacket(void *pkt, unsigned size)
{
	return send(clientSocket, (const char *) pkt, size, 0);
	// return sendto(ClientFd, (const char *)pkt, size, 0,(struct sockaddr *)&TcpClientAddr, sizeof(TcpClientAddr));
}

int ImxBoardGetData(void *pkt, unsigned maxSize , int timeoutMs )
{
	fd_set fds;
	struct timeval tv;

    // basic select call setup
	// FD_ZERO(&fds);
	// FD_SET(ClientFd, &fds);

	// Set up timeout
	tv.tv_sec = timeoutMs / 1000;
	tv.tv_usec = (timeoutMs * 1000) % 1000000;

	// if (select(ClientFd + 1, &fds, NULL, NULL, &tv) > 0)
	// {
		// copy packet into buffer
		return recv(clientSocket, (char *)pkt, maxSize, 0);
		// return recvfrom(ClientFd, (char *)pkt, maxSize, 0, NULL, NULL);
	// }

	// Timeout
	return ERR;
}

int sendFile(FILE* fp, char* buf, int bufSize , uint16_t* length  )
{
#define nofile "File Not Found!"
//	char tmp[10] = { 0 } ;
	int i, len;
    char ch;
    if (fp == NULL) {
        strcpy(buf, nofile);
        len = strlen(nofile);
        buf[len] = EOF;
        return 1;
    }

    for (i = 0; i < bufSize; i++) {
    	ch = fgetc(fp);
        if (ch == EOF)
        {
        	*length = i ;
            return 1;
        }
//        printf("%d - %c \n" ,i, ch );
        buf[i] = ch;
    }
    *length = i ;
    return 0;
}


int ImxBoardDownloadFile( char* fileName )
{
#define SEND_BUF_SIZE 64
	char net_buf[SEND_BUF_SIZE+1] = { 0 } ;
	FILE * fp;
	uint16_t len = 0 ;


	fileName = (char*)"test.txt" ;
//	fileNameLength = strlen( fileName );
	printf("file name of user is %s \n" , fileName );

	fp = fopen (fileName, "r+");
	if( fp )
	{
		printf("success to open file \n");
	}
	else
		printf("fail to open file \n");




	// download process
    while (1)
    {
        // process
        if (sendFile(fp, net_buf,SEND_BUF_SIZE , &len))
        {
        	// send the last packet
			send(clientSocket, net_buf, SEND_BUF_SIZE, 0);
            // sendto(ImxBoardFd, net_buf, SEND_BUF_SIZE,0,(const struct sockaddr *) &TcpClientAddr,sizeof(TcpClientAddr));
            break;
        }

        // send
		send(clientSocket, net_buf, SEND_BUF_SIZE, 0);
        // sendto(ImxBoardFd, net_buf, SEND_BUF_SIZE,0,(const struct sockaddr *) &TcpClientAddr,sizeof(TcpClientAddr));
        memset( net_buf , 0 , sizeof(net_buf));
    }

    if (fp != NULL)
    {
        fclose(fp);
    }

    return 0 ;
}




int ImxBoard_Sendbuffer( char* buffer , int size )
{
//	printf("send packet with %d \n" , size );
	send(clientSocket, buffer, size, 0);
	// sendto(ImxBoardFd, buffer, size , 0, (const struct sockaddr *) &TcpClientAddr,sizeof(TcpClientAddr));
	return 0 ;
}

void* SendFileToRemote( void* args )
{
	uint32_t motionSize = radar.params.fpsMotion * radar.params.motionTime * 4 * 390 ;
	uint32_t postureSize = radar.params.nScanPosture * 4 * 390 ;
	uint32_t repsSize    = radar.params.fpsRespiration * radar.params.respTimeShort * 4 * 390 ;
	float sizeArray[4] = { STOP_SIZE , MOTION_SIZE , POSTURE_SIZE , RESPIRATION_SIZE } ;
//	uint32_t sizeArray[4] = { STOP_SIZE , motionSize , postureSize , repsSize } ;


	while(1)
	{
		if( packetIsReady )
		{
			sizeArray[0] = 0 ;
			sizeArray[1] = radar.params.fpsMotion * radar.params.motionTime * 4 * 390 ; ;
			sizeArray[2] = radar.params.nScanPosture * 4 * 390 ;
			sizeArray[3] = radar.params.fpsRespiration * radar.params.respTimeShort * 4 * 390 ;

			printf("radar.params.fpsMotion %d --- radar.params.motionTime %f  ::: %f \n" ,  radar.params.fpsMotion , radar.params.motionTime , sizeArray[1] ) ;
			printf("radar.params.nScanPosture %d ::: %f \n" ,  radar.params.nScanPosture , sizeArray[2] ) ;
			printf("radar.params.fpsRespiration %d radar.params.respTimeShort %d  :: %f\n" ,  radar.params.fpsRespiration , radar.params.respTimeShort , sizeArray[3] ) ;

			gettimeofday(&tpStop,0);

//			f1 = ( float( tpStop.tv_sec-tpStart.tv_sec)+ (float)(tpStop.tv_usec)/1000000 ) -  ((float)(tpStart.tv_usec)/1000000) ;
//			printf ("elpase %f time \n" , f1 );
			pthread_mutex_lock(&SendFileMutex);
			packetIsReady = false ;

			if( radar.state == INIT_STATE_SEND_ACK )
			{
				ackMsg AckMessage = { 0 } ;
				AckMessage.status = 1 ;
				AckMessage.bins = 389 ; //(uint16_t)radar.bins ;
				SendPacket( (char*)&AckMessage , sizeof(ackMsg) ) ;
				radar.state = OPERATION_STATE ;
				RadarInitIndex();
			}
			else
			{
				// send file
				sendingNow = true;
				printf("send file data type %d size %d \n" , radar.DataType , sizeArray[radar.DataType] );
				SendPacket( (char*)image , sizeArray[radar.DataType] ) ;
				sendingNow = false;
			}
			pthread_mutex_unlock(&SendFileMutex);
//			printf("finish to Send \n");
//			radar.NoOfLoops-- ;
//			if( radar.NoOfLoops )


		}
	}
	return NULL;

}


void* SendPacket( char* buffer , uint32_t bufferSize )
{
	char net_buf[499200+4] = { 0 } ;
//	int size ;
//	int *len = (int*)net_buf ;
	uint16_t NoOfByteToSend = 0 ;
	uint32_t SentByteCount  = 0 ;
	char *pBuffer = NULL;
//	float* pBuf = (float*)buffer ;


	// first packet sending length
/*
	*len = bufferSize ;
	pBuffer = &net_buf[0] ;
	ImxBoard_Sendbuffer( pBuffer , 4 ) ;
*/

#if 1
	pBuffer = &net_buf[0] ;
	// download process
	memcpy( pBuffer , buffer , bufferSize );
	ImxBoard_Sendbuffer( pBuffer , bufferSize ) ;
#else
    ImxBoard_Sendbuffer( buffer , bufferSize ) ;
    printf("buffer size %d send byte count %d \n" , bufferSize , SentByteCount );
#endif
    printf("buffer size %d send byte count %d \n" , bufferSize , SentByteCount );
	return 0;
}

void* GetCommandFromRemote( void* args )
{
	fd_set fds;
	ProcessCharacters Child_1 ;
//	int listenfd ;
	socklen_t len;
//	struct sockaddr_in  cliaddr;



	if (ImxBoardOpenSocketInit() == -1){
		return NULL;
	}
//	ClientFd = ImxBoardFd ;

	printf("Start +++++++++ \n");

/*
	radar.fpXml = fopen( "initXml.xml" , "w+" );
	if ( !radar.fpXml )
	{
		printf("Error opening %s\n", "initXml.xml" );

	}
*/

//	parseDoc( "novelda_params.xml" , &radar.params ) ;
//	parseDoc( "initXml.xml" , &radar.params ) ;
	printf("\nStarting GetCommandFromRemote loop \n");
	while(1)
	{
	    // basic select call setup
		// FD_ZERO(&fds);
		// FD_SET(ImxBoardFd, &fds);

		// if (select(ImxBoardFd + 1, &fds, NULL, NULL, NULL) > 0)
		// {
			// if( FD_ISSET(ImxBoardFd, &fds ))
			// {
				//receive the datagram
				len = sizeof(TcpClientAddr);
				int n = recv(clientSocket, Child_1.buffer, sizeof(Child_1.buffer), 0);
				    if (n == -1) {
						std::cerr << "recv() failed: " << strerror(errno) << std::endl;
						// Handle the error
					}
				// int n = recvfrom(ImxBoardFd, Child_1.buffer, sizeof(Child_1.buffer),0, (struct sockaddr*)&TcpClientAddr,&len); //receive message from server
				printf("Received function ---- read %d byte from remote \n" , n );
//				Child_1.buffer[n] = '\0';
//				puts(Child_1.buffer);

				RemoteAnalzeRemoteCommand( Child_1.buffer  , n );
//				Child_1.buffer[0] += 1;

				// send the response
//				sendto(ImxBoardFd, Child_1.buffer, 1 , 0, (const struct sockaddr *) &TcpClientAddr,sizeof(TcpClientAddr));
//				sendto(ImxBoardFd, Child_1.buffer, 1, 0,(struct sockaddr*)&TcpClientAddr, sizeof(TcpClientAddr));
/*
				printf("ronen\n");
				memset(Child_1.buffer , 0 , sizeof(Child_1.buffer));
				ImxBoardGetPacket( Child_1.buffer , sizeof(Child_1.buffer));
//				SendPacket( (char*)image , sizeof(image) ) ;
				RemoteAnalzeRemoteCommand( Child_1.buffer );
*/
			// }
		// }

	}

	printf("parent finish \n");
	return NULL;

}

void RemoteAnalzeRemoteCommand( char *buffer , int size )
{
	remoteMsg *msg = (remoteMsg*)buffer ;

	char array[200000] = { 1 } ;

	printf("\nGetting remote command: %c\n", msg->cmd);
	printf("r start %f \n baseline %d \n", msg->range.rstart, msg->baselineNo );

	switch( msg->cmd )
	{
	case 'm':
	case 'M':
		gettimeofday(&tpStart,0);
		radar.DataType = MOTION ;
		radar.NoPacketToReceive = radar.params.motionTime * radar.params.fpsMotion ;
//R		RadarChangeRange( msg->range.rstart ) ;
		printf("r start %f \n", msg->range.rstart );
		printf("radar.NoPacketToSend %d radar.params.motionTime %f radar.params.fpsMotion %d \n" , radar.NoPacketToReceive , radar.params.motionTime , radar.params.fpsMotion);
//		TaskRadarSetAntenna( (uint8_t)0 );
/*//R*/		TaskRadarSetAntenna( msg->baselineNo );
/*//R*/		RadarStartStop(MOTION);
		break;
	case 'p' :
	case 'P' :
		radar.DataType = POSTURE ;
		radar.NoPacketToReceive = radar.params.nScanPosture ;
		radar.postureBaselineIndex = 0 ;
		TaskRadarSetAntenna( radar.postureBaselineIndex );
		RadarChangeRange( msg->range.rstart ) ;
		printf("r start %f \n", msg->range.rstart );
		RadarStartStop(POSTURE);

		break ;
	case 'r' :
	case 'R' :
		printf("respiration \n");
		radar.DataType = RESPIRATION ;
		radar.NoPacketToReceive = radar.params.respTime * radar.params.fpsRespiration ;
		RadarChangeRange( msg->range.rstart ) ;
		printf("r start %f \n baseline %d ", msg->range.rstart, msg->baselineNo );
		TaskRadarSetAntenna( msg->baselineNo );
		RadarStartStop(RESPIRATION);
		break ;
	case 'I' :

#if 0
		radar.fpXml = fopen( "init.xml" , "w+" );
		if ( !radar.fpXml )
		{
			printf("Error opening %s\n", "initXml.xml" );

		}
		else
		{
			fwrite( buffer+1, size-1 , 1 , radar.fpXml  ) ;
			printf("size is %d \n" , size );
			fclose( radar.fpXml );
			parseDoc ((char*)"init.xml" , &radar.params );
			RadarConfigParameters( &radar.params ) ;
			// start radar to get no Of bins
			radar.state = INIT_STATE ;
			RadarStartStop(MOTION);
		}
#else

		// start radar to get no Of bins
		radar.state = INIT_STATE ;
		RadarStartStop(MOTION);

		printf("ronen command I \n");

#endif
		break;
	default:
		printf("shalom\n");
		for(int x = 0 ; x < 200 ; x++ )
		{
			memset( (void*)&array[x*1000] , 0x30+(x%10) , 1000 );
		}
		for( int y = 0 ; y < 20 ; y++)
		//memcpy( array , "Ronen Aharoni" , strlen("Ronen Aharoni") );
			SendPacket( (char*)&array[y*10000] , 10000 ) ;
		break;
	}
}
