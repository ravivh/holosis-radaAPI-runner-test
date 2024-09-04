/*
 * main.h
 *
 *  Created on: Sep 10, 2019
 *      Author: echocare
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <stdio.h>
#include "common.h"


typedef enum
{
	INIT_STATE = 3 ,
	INIT_STATE_SEND_ACK ,
	OPERATION_STATE ,
}radarState;

typedef enum
{
	STOP,
	MOTION ,
	POSTURE ,
	RESPIRATION,
}DataType;

typedef enum
{
	STOP_SIZE        = 0 ,
	MOTION_SIZE      = 800*4*390 ,
	POSTURE_SIZE     = 64*4*390  ,
	RESPIRATION_SIZE = 40*4*390  ,
}DataTypeSize;

#define DEMO 1
#define DEBUGER 0

void signalHandler(int);

#endif /* SRC_MAIN_H_ */


