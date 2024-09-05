#include <stdio.h>
#include <stdlib.h>
#include "RadarApi.h"
#include "taskRadar.h"


// TODO: need to make this thread-safe
RadarApiParams radar_params;

// types are based on x4driver api not internal x4 parameters structure
// TODO: add struct for errors to include x4driver errors
int radar_init(int novelda_freq, int transmit_gain, int dacMin,
	           int dacMax, float32_t tZeroNs, int iterations, int debug)
{
    /*
    The radar_init function is called by the api to initialize the radar task. Performs full initialization of the radar
    including interrupt, gpio, spi and set the parameters for the radar which don't change during the device operation.
    Should only be called once! To change constant parameters, call radar_change_parameters.
    */
    uint32_t status = taskRadar(NULL);
    printf("Task Radar Status: %d\n", status);
    if (status != 0)
    {
        printf("Failed to initialize radar task\n");
        return FAILED_TO_INITIALIZE_RADAR_TASK;
    }
    status = radar_change_parameters(novelda_freq, transmit_gain, dacMin, dacMax, tZeroNs, iterations);
    return status;
}

int radar_change_parameters(int novelda_freq, int transmit_gain, int dacMin,
	                        int dacMax, float32_t tZeroNs, int iterations)
{
    /*
    The radar_change_parameters function is called by the api to change the radar parameters.
    This function will set the Novelda parameters which are constant over the course of the radar operation.
    */
    uint32_t status = 0;
   // TODO: think you should add feedback to python that the parameters are set
    // at least for debug - you can redirect printf to python which would work here
    radar_params.params.dacMin = (uint16_t)dacMin;
    radar_params.params.dacMax = (uint16_t)dacMax;
    radar_params.params.iterations = (uint8_t)iterations;
    
    radar_params.params.tZeroNs = tZeroNs;
    switch (novelda_freq)
    {
        case 3:
            radar_params.params.novelda_freq = TX_CENTER_FREQUENCY_EU_7_290GHz;
            break;
        case 4:
            radar_params.params.novelda_freq = TX_CENTER_FREQUENCY_KCC_8_748GHz;
            break;
        default:
            printf("Unrecognized frequency! Defaulting to KCC freq");
            radar_params.params.novelda_freq = TX_CENTER_FREQUENCY_KCC_8_748GHz;
            break;
    }
    switch (transmit_gain)
    {
        case 0:
            radar_params.params.transmitGain = TX_POWER_OFF;
            break;
        case 1:
            radar_params.params.transmitGain = TX_POWER_LOW;
            break;
        case 2:
            radar_params.params.transmitGain = TX_POWER_MEDIUM;
            break;
        case 3:
            radar_params.params.transmitGain = TX_POWER_HIGH;
            break;
        default:
            printf("Unrecognized power! Defaulting to maximum");
            radar_params.params.transmitGain = TX_POWER_HIGH;
            break;
    }
    printf("Configuring Constant Radar Parameters\n");
    status = RadarConfigConstantParameters(&radar_params.params);
    if (status != 0)
    {
        printf("Failed to initialize radar task\n");
        return FAILED_TO_INITIALIZE_RADAR_TASK;
    }
    printf("Status of constant parameter configuration: %d\n", status);
    status = StopRadar();
    printf("Status of stopRadar: %d\n", status);
    if(status != 0)
    {
        printf("Failed to stop radar\n");
        return FAILED_TO_STOP_RADAR;
    }
    return 0;
}

int radar_request(int* antenna_numbers, int antenna_len, int repetitions,
 				  float32_t* write_buffer, float32_t Rstart_m, float32_t startToStop_m,
                  uint8_t prfDiv, uint16_t pps, float32_t fps, uint8_t downconversion_enable,
                  uint8_t* finished)
{
	/*
	The radar_request function is called by the api to request radar data from the X4 radar.
	Once a request is received, the function will set the Novelda parameters
	*/
    // want to see if the setting persists from previous call
    printf("prfDiv last: %d\n", radar_params.params.prfDiv);
    radar_params.repetitions = (uint16_t)repetitions;
    radar_params.antenna_num = (uint16_t)antenna_len;
    radar_params.params.Rstart_m = Rstart_m;
    radar_params.params.startToStop_m = startToStop_m;
    radar_params.params.prfDiv = prfDiv;
    radar_params.params.pps = pps;
    radar_params.params.fps = fps;
    radar_params.params.downconversion_enable = downconversion_enable;
    radar_params.antennas = antenna_numbers;
    radar_params.write_buffer = write_buffer;
    radar_params.packet_number = 0;
    radar_params.antenna_index = 0;
    radar_params.frameReady = finished;
    TaskRadarSetAntenna(antenna_numbers[0]);
    printf("Antenna number: %d\n", antenna_numbers[0]);
    radar_params.total_packets = radar_params.repetitions * radar_params.antenna_num;
    // TODO: configuring fps starts the radar?
    printf("Configuring Dynamic Radar Parameters\n");
    uint32_t status = RadarConfigDynamicParameters(&radar_params.params);
    if (status != 0)
    {
        printf("Error status: %d\n", status);
        printf("Failed to get data. Failed to configure radar parameters\n");
        return FAILED_TO_CONFIGURE_NEW_PARAMETERS;
    }

    while(*radar_params.frameReady == 0)
    {
        // wait for data to be ready
    }
    
    for(int i=0; i < radar_params.total_packets; i++)
    {
        printf("Packet %d: %d\n", i, radar_params.packet_iter[i]);
    }

    return 0;
}
