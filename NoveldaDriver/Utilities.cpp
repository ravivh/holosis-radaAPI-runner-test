/*
 * Utilities.cpp
 *
 *  Created on: Feb 24, 2019
 *      Author: echocare
 */


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

/*
#include "RadarHandler.h"
#include "hostInterfaceCommon.h"
#include "BackGroundTask.h"
#include "mrmIf.h"
#include "mrm.h"
*/


/*
int Utilites_shell( int argc , char** argv )
{
	char cmd[10] = { 0 } ;
	mrm_uint32_t mode = 0  ;
	mrm_uint16_t direction = 0 , mask = 0x7C00 , gpio = 0 ;


    struct timeval tp ;
	time_t curtime;
	struct tm *t;

	gettimeofday(&tp,0);
	curtime = tp.tv_sec;
	t = localtime(&curtime);

	printf (": %02d:%02d:%02d:%03d\n",(int)t->tm_hour,(int)t->tm_min, (int)t->tm_sec,(int)tp.tv_usec/1000);

//	ImxInitializeRadar();
	while( 1 )
	{
		printf("enter command :");
		scanf("%s" , cmd );

		if( strcmp( cmd , "gc")  == 0 )
		{

			mrmConfiguration config1 ;
			int i = 0 ;
			mrmConfigGet(&config1);
		    // print out configuration
		    printf("\nConfiguration:\n");
		    printf("\tnodeId: %d\n", config1.nodeId);
		    printf("\tscanStartPs: %d\n", config1.scanStartPs);
		    printf("\tscanEndPs: %d\n", config1.scanEndPs);
		    printf("\tscanResolutionBins: %d\n", config1.scanResolutionBins);
		    printf("\tbaseIntegrationIndex: %d\n", config1.baseIntegrationIndex);
		    for (i = 0 ; i < 4; i++)
		    {
		        printf("\tsegment %d segmentNumSamples: %d\n", i, config1.segmentNumSamples[i]);
		        printf("\tsegment %d segmentIntMult: %d\n", i, config1.segmentIntMult[i]);
		    }
		    printf("\tantennaMode: %d\n", config1.antennaMode);
		    printf("\ttxGain: %d\n", config1.txGain);
		    printf("\tcodeChannel: %d\n", config1.codeChannel);

		}

		if( strcmp( cmd , "sc")  == 0 )
		{

			mrmConfiguration config1 ;
			char parameter ;
			int value = 0 ;
			mrmConfigGet(&config1);
			while(1)
			{
				printf("please choose parameter to modify p-pii g-gain c-channel s-start e-end q-quit\n");
				scanf("%c %d" , &parameter , &value );
				switch(parameter)
				{
				case 'p':
					config1.baseIntegrationIndex = value;
					break;
					break;
				case 'g':
					config1.txGain = value;
					break;
				case 'c':
					config1.codeChannel = value ;
					break;
				case 's':
					config1.scanStartPs = value;
					break;
				case 'e':
					config1.scanEndPs = value;
					break;
				case 'q':
					break;
				default :
					break;
				}
				if( parameter == 'q')
					break;
			}
			// write updated config to radio
			if (mrmConfigSet(&config1) != 0)
			{
				printf("Time out waiting for set config confirm.\n");
				return ERR ;//mrmSampleExit();
			}
		}


		if( strcmp( cmd , "gcgpio")  == 0 )
		{
			mrmGpioConfigGet();
		}
		if( strcmp( cmd , "scgpio")  == 0 )
		{
			utility_getFromUserGpioMode( &mode , &direction );
			mrmGpioConfigSet( mode , direction );
		}
		if( strcmp( cmd , "ggpio") == 0)
		{
			mrmGpioGet();
		}
		if( strcmp( cmd , "sgpio") == 0)
		{
			utility_getFromUserGpioVal( &gpio) ;
			mrmGpioSet( gpio , mask );
		}
		if( strcmp( cmd , "start") == 0)
		{
//			StartRadarSession( 400, 0 , NULL );
		}
		if( strcmp( cmd , "bye") == 0 )
			break ;
		printf("%s\n", cmd );
		memset(cmd , 0 , sizeof(cmd));

	}
	return 0 ;

}
*/


void StringReplaceChars( char* string , char findChar , char ReplaceChar )
{
	char *tmp = string ;
//	printf("before %s \n" , string );
	while( *tmp != '\0' )
	{
		if( *tmp == findChar )
			*tmp = ReplaceChar ;
		tmp++;
	}
//	printf("after %s \n" , string );
}

int PipeGetMessage(int pipeFd , char* buffer , int size )
{
	fd_set fds;
//	struct timeval tv;

    // basic select call setup
	FD_ZERO(&fds);
	FD_SET(pipeFd, &fds);

	// Set up timeout
//	tv.tv_sec = timeoutMs / 1000;
//	tv.tv_usec = (timeoutMs * 1000) % 1000000;

	if (select(pipeFd + 1, &fds, NULL, NULL, NULL) > 0)
	{
		// copy packet into buffer
		return read( pipeFd , buffer, size);
	}

	// Timeout
	return (-1);//ERR;
}



uint32_t LightSpeedConvertMeterToTime( double meter )
{
	long light_speed = 299792458;
	float time       = (float)2*meter/(float)light_speed ;
//	printf(" input %f  calc %.12f multiply %d \n" , meter , time , (int)(time*1000000000000) );

	return ( uint32_t ) ( time * 1000000000000 );
}


double Distance( double x , double y , double z )
{
	printf("power( %f ,2 ) - %f \n" , x , pow(x,2) );

	return sqrt( pow( x , 2 ) + pow( y , 2 ) + pow( z , 2 ) )  ;
}

/*
 Returns the current time.
*/

void time_stamp( void )
 {
	char *timestamp = (char *)malloc(sizeof(char) * 16);

	time_t ltime;
	ltime=time(NULL);
	struct tm *tm;
	tm=localtime(&ltime);

	printf("%04d:%02d:%02d:%02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	free( timestamp );
 }




