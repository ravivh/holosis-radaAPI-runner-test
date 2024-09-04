/*
 *********************************************************************
 *																	 *
 *        Copyright (c) 2023 Echocare Technologies Ltd               *
 * 																	 *
 *********************************************************************

 *	Date: Oct 28, 2019

 *  File name: Common.h

 *  Owner :
 *
 *  Description:
 *
 */


/*
 * Common.h
 *
 *  Created on: Oct 28, 2019
 *      Author: echocare
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include "hostInterfaceCommon.h"
#include <mqueue.h>
#include <stdbool.h>
#include <pthread.h>
#include "hostInterfaceMRM.h"
#include "x4driver.h"

#define EU_MODE 0

extern X4Driver_t *x4driver ;
extern pthread_t  InterruptTid;


#define X_LEN 51
#define Y_LEN 51
#define N_POSTURE_FEATURES 10
#define N_POSTURE_TREES 15

#define MAX_LOCATIONS 15
#define LOCATIONS_STR_LEN 20
#define PRINT_FOR_DEBUG 0

extern char *OutOfHome;
extern char *OutOfRoom;


typedef struct
{
	uint16_t udpBufferLength   ;
	float    Rstart_m          ;        // 1 (null)
	float    startToStop_m     ;        // 3.5 (null)
	uint8_t  prfDivMotion      ;                    // 11 (null)
	uint8_t  prfDivPosture     ;                    // 11 (null)
	uint8_t  prfDivRespiration ;                    // 11 (null)
	uint8_t  transmitGain      ;
	uint16_t ppsMotion         ;
	uint16_t ppsPosture        ;
	uint16_t ppsRespiration    ;
	uint8_t  iterationsMotion  ;
	uint8_t  iterationsPosture ;
	uint8_t  iterationsRespiration ;
	uint16_t fpsMotion         ;
	uint16_t fpsPosture        ;
	uint16_t fpsRespiration    ;
	uint16_t dacMin            ;
	uint16_t dacMax            ;
	float    tZeroNs           ;               // 17  (null)
	float    motionTime        ;
	uint8_t  respTime          ;
	uint8_t  respTimeShort     ;
	uint16_t nScanPosture      ;
	uint8_t  MotionBaseLine    ;
	float    RstartMinThrsh    ;
	uint8_t  frequencyType     ;

}NoveldaRadarParams;



typedef struct RadarParams
{
	uint8_t  DataType ;
	uint16_t NoPacketToReceive ;
	uint16_t NoOfLoops ;
	uint16_t postureBaselineIndex ;
	FILE *fpXml ;
	NoveldaRadarParams params ;
	uint8_t state ;
	uint32_t bins ;
	uint8_t frameReady ;

}RadarParametes ;


typedef struct locationXY_Struct
{

	float x;
	float y;

}locationXY;

typedef struct _TrainingStruct
{

	int		numberOfxy;
	float 	*xyMatrix;
	int		N_Locations;
	char 	*locationsNames[MAX_LOCATIONS];
	char transitionMatrix[MAX_LOCATIONS][MAX_LOCATIONS];
	int allowedLocation[MAX_LOCATIONS];
	int sofaIsBed;
}trainingVariables;


typedef enum
{
	FG_COG_OPERATION_EVENT 				            = 1 ,
	FG_COG_ACQ_HUMEN_DETECT_EVENT 					= 2 ,
	FG_COG_ACQ_SCAN_RETRY                           = 3 ,
	FG_COG_ACQ_FINISH_ALL_RANGE_EVENT				= 4 ,
	FG_COG_RESP_EVENT                               = 5 ,
	FG_COG_VISITORS_EVENT							= 6 ,
	FG_COG_ACQ_HUMEN_DETECT_NOT_ALLOWED_EVENT       = 7 ,
}fgtoCogEvent;


typedef enum
{
	LOCATION_CLASSIFIER_OPERATION = 0 ,
	LOCATION_CLASSIFIER_ACQUISITION = 1 ,
}locationClassifierEnum ;


typedef enum
{
	ALLOWED = 0 ,
	NO_ALLOWED = -1 ,
}allowStatus;

extern RadarParametes radar  ;
extern void BackGround_SetPacketReadyFlag( void );
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

float LightSpeedConvertTimeToMeter( float time );



#endif /* COMMON_H_ */
