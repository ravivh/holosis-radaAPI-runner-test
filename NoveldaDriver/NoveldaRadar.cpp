#include <iostream>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <stdio.h>
#include <string.h>
#include <csignal>    // For signal handling
#include <cstdlib>    // For exit(), EXIT_SUCCESS
#include <unistd.h>   // For close()

//#include <wiringPi.h>
#include "wiringx.h"
#include "taskRadar.h"
#include "x4driver.h"
#include "RemoteConnection.h"
#include "NoveldaRadar.h"

#define DEBUG

using namespace std;
float image[MAX_SIZE_OF_PACKETS][390] = { 0 } ;
bool packetIsReady = 0;
pthread_mutex_t SendFileMutex;

RadarParametes radar  = { 0 } ;



int maintest(int argc, char *argv[])
{
	pthread_t tid;
	pthread_t tidWriteToFile ;
	pthread_t tidSendToRemote;
	pthread_t tidGetCmdFromRemote;


//	char c = 0 ;
	char* commandArray[20] = { 0 } ;
	int NoOfParams = 0 ;
	int baseline = 0 ;

	radar.NoPacketToReceive = 400 ;
	isConnectionEstablished = false;

    printf("Imx.6 x4driver start to work!\n""""""\n\n");
	if( pthread_create(&(tidGetCmdFromRemote), NULL, &GetCommandFromRemote, NULL) )
		printf("fail to create Foreground thread tidGetCmdFromRemote \n") ;
	
	printf("\n isConnectionEstablished = %s", isConnectionEstablished? "True":"False");
	while(1){
		if (isConnectionEstablished){
			printf("~~~~~~connection established!~~~~~~");
			if( pthread_create(&(tid), NULL, &taskRadar, NULL) )
				printf("fail to create Foreground thread tid\n") ;

			if( pthread_create(&(tidWriteToFile), NULL, &taskWriteToFile, NULL) )
				printf("fail to create Foreground thread tidWriteToFile\n") ;

			if( pthread_create(&(tidSendToRemote), NULL, &SendFileToRemote, NULL) )
				printf("fail to create Foreground thread tidSendToRemote\n") ;

			break;
		}
	}



	if (pthread_mutex_init(&SendFileMutex, NULL) != 0)
	    {
	        printf("\n mutex init has failed\n");
	        exit(0);
	    }


	while( task_state == 1 )
	{

		NoOfParams = 0 ;


		GetInputFromUser(  &NoOfParams , commandArray );

		if( commandArray[0] != NULL )
		{

			if( !strcmp( commandArray[0]  , "q") )
			{
				closeSockets();
				return 0;
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "txoff") )
			{
				RadarChangeTxPower( TX_POWER_OFF );
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "txlow") )
			{
				RadarChangeTxPower( TX_POWER_LOW );
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "txmed") )
			{
				RadarChangeTxPower( TX_POWER_MEDIUM );
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "txhigh") )
			{
				RadarChangeTxPower( TX_POWER_HIGH );
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "stop") )
			{
				RadarStartStop(STOP);
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "start") )
			{
				RadarStartStop(MOTION);
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "intclr") )
			{
				RadarClearInterrupt();
	//			c = 0xff ;
			}
			if( !strcmp( commandArray[0]  , "bs") )
			{
				if( NoOfParams > 1 )
				{
					sscanf(commandArray[1] , "%d" , &baseline);
					printf("baseline is %d \n", baseline);
					TaskRadarSetAntenna( (uint8_t)baseline );
				}
			}
			if( !strcmp( commandArray[0]  , "int") )
			{
				int intEnable = 0;
				if( NoOfParams > 1 )
				{
					sscanf(commandArray[1] , "%d" , &intEnable);
					printf("%s interrupt \n", intEnable ? "Enable" : "Disable");
					RadarInterruptEnableDisable(intEnable);
				}
			}
			if( !strcmp( commandArray[0]  , "gpio8") )
			{
				printf("ronen \n");
				int state ;
				if( NoOfParams > 1 )
				{
					printf("aharoni %s \n" , commandArray[1]);
					sscanf(commandArray[1] , "%d" , &state);
					printf("state is %d \n", state);
					digitalWrite(8,(digital_value_t)state);
				}
			}
			if( !strcmp( commandArray[0]  , "freq") )
			{
				printf("freq ddx \n");
				int state ;
				xtx4_tx_center_frequency_t tx_center_frequency ;
				if( NoOfParams > 1 )
				{
					printf("freq %s \n" , commandArray[1]);
					sscanf(commandArray[1] , "%d" , &state);
					printf("state is %d \n", state);
					tx_center_frequency = state ? TX_CENTER_FREQUENCY_KCC_8_748GHz : TX_CENTER_FREQUENCY_EU_7_290GHz ;
					RadarChangeTxFreq( tx_center_frequency ) ;
				}
			}
			if( !strcmp( commandArray[0]  , "mux") )
			{
				printf("ronen \n");
				int pin ;
				if( NoOfParams > 1 )
				{
					printf("aharoni %s \n" , commandArray[1]);
					sscanf(commandArray[1] , "%d" , &pin);
					printf("state is %d \n", pin);
					printPinMux( pin ) ;
				}
			}
		}
	}

 //   std::thread taskRadarThread(taskRadar);
 //   taskRadarThread.join();

    printf("raspbian_x4driver done.\n");
    return 0;
}

