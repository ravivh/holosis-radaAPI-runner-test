/*
 * Utilities.h
 *
 *  Created on: Feb 24, 2019
 *      Author: echocare
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <arpa/inet.h>


/*int Utilites_shell( int argc , char** argv );*/

void StringReplaceChars( char* string , char findChar , char ReplaceChar );

int PipeGetMessage(int pipeFd , char* buffer , int size );

uint32_t LightSpeedConvertMeterToTime( double meter );
double Distance( double x , double y , double z );

void time_stamp( void ) ;

#endif /* UTILITIES_H_ */
