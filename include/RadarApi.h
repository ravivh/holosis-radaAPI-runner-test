#ifndef RADAR_API_H_
#define RADAR_API_H_

#ifdef __cplusplus
#include "x4driver.h"
#include "common.h"

typedef enum
{
	RADAR_SUCCESS = 0,
	FAILED_TO_INITIALIZE_RADAR_TASK = -1,
	FAILED_TO_CONFIGURE_NEW_PARAMETERS = -2,
	FAILED_TO_GET_DATA = -3,
	FAILED_TO_STOP_RADAR = -4,
	RADAR_NO_INIT = -5,
	
} radar_api_error_codes;

typedef struct return_codes_t
{
	uint8_t status;
	radar_api_error_codes error_code;
} return_codes_t;

extern "C" {
int radar_init(int novelda_freq, int transmit_gain, int dacMin,
	           int dacMax, float32_t tZeroNs, int iterations, int debug);

int radar_request(int* antenna_numbers, int antenna_len, int repetitions,
 				  float32_t* write_buffer, uint8_t* finished);

int radar_change_parameters(int novelda_freq, int transmit_gain, int dacMin,
	                        int dacMax, float32_t tZeroNs, int iterations);

int set_request_params(float32_t Rstart_m, float32_t startToStop_m,
                       uint8_t prfDiv, uint16_t pps, float32_t fps,
                       uint8_t downconversion_enable, uint32_t* bins, uint32_t chunk_size);

int stop_radar();
}
#endif

#endif