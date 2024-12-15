#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <wiringPi.h>
//#include <wiringPiSPI.h>
#include "wiringx.h"
#include "radar_hal.h"
#include "taskRadar.h"
#include "x4driver.h"
#include "xep_hal.h"
#include "RemoteConnection.h"
#include "Utilities.h"
#include "NoveldaRadar.h"
#include "common.h"


extern "C" {
#include <imx6dqrm.h>
}
extern RadarApiParams radar_params;

// make it as large as the most bins you will have 1536
float32_t garbage_buffer[1536];

int readFlag = 0 ;

// https://github.com/novelda/Legacy-SW/blob/master/Examples/X4Driver_RaspberryPi/src/Radar/taskRadar.cpp

void TaskRadarGpioInit( void );
void TaskRadarSetAntenna( uint8_t AntennaBaseState );
void TaskRadarResetRadar( void );


volatile xtx4driver_errors_t x4_initialize_status = XEP_ERROR_X4DRIVER_UNINITIALIZED;
float32_t data_frame_normolized[2][400] = { 0 } ;
uint8_t buffer_index = 0;
uint8_t WriteDataFlag = 0 ;
extern bool sendingNow;
X4Driver_t *x4driver = NULL;


char logFileName[100] = { 0 } ;
char logFileName2[100] = { 0 } ;
FILE *fpFile = NULL;
FILE *fpFile2 = NULL;
int frameIndex = 0 ;

#define DEBUG 1

using namespace std;
std::recursive_mutex x4driver_mutex;

typedef struct
{
    //TaskHandle_t radar_task_handle;
    radar_handle_t *radar_handle; // Some info separating different radar chips on the same module.
} XepRadarX4DriverUserReference_t;

typedef struct
{
    //XepDispatch_t* dispatch;
    X4Driver_t *x4driver;
} RadarTaskParameters_t;

// TODO; to change
void x4driver_data_ready(void)
{
//	printf("ronen aharoni\n");
	static int once = 0 ;
	static uint32_t counterRx = 0 ;
//	static digital_value_t toggle = LOW ;
    uint32_t status = XEP_ERROR_X4DRIVER_OK;
    uint32_t bin_count = 0;
     uint32_t frame_counter = 0;
    uint8_t down_conversion_enabled = radar_params.params.downconversion_enable;
//   static int ronenDebug = 0 ;

    //printf("callback reached !\n");
    x4driver_get_frame_bin_count(x4driver, &bin_count);
    x4driver_get_downconversion(x4driver, &down_conversion_enabled);
    if( once == 0 )
    {
    	once = 1 ;
    	printf("no of bins %d \n" , bin_count );
    }
//    digitalWrite( X4_TEST_PIN, HIGH);
    uint32_t fdata_count = bin_count;
    if (down_conversion_enabled == 1)
    {
        fdata_count = bin_count * 2;
    }
   
    uint32_t frame_index = radar_params.packet_number;
    radar_params.packet_iter[radar_params.packet_number] = radar_params.packet_number;
// TODO: should data be read before stopping radar? Is there a way to flush the buffer
// need to understand when the interrupt is raised and what the implications are of stopping the radar
    if( radar_params.packet_number++ >= radar_params.total_packets - 1)
    	StopRadar();
    else
    {
        if( radar_params.antenna_num > 1 )
        {
        	StopRadar();
            radar_params.antenna_index++;
        	if( radar_params.antenna_index == radar_params.antenna_num)
        		radar_params.antenna_index = 0 ;      	
        	TaskRadarSetAntenna( radar_params.antennas[radar_params.antenna_index] );
            /***************set fps, this will trigger data output***************/
            status = x4driver_set_fps(x4driver, radar_params.params.fps);
            if (status != XEP_ERROR_X4DRIVER_OK)
            {
                printf("Error in x4driver_set_fps\n");
                printf("Error code=%d\n", status);
            }
        }
    }
    // this is in production code - maybe it's a type of buffer flush?
    // as in if you get an additional callback after the radar is stopped it could be invalid data you
    // need to handle
    // printf("fdata count %d\n", fdata_count);
    // TODO - what should I do if it fails here? need to return something at least
    // also should it be <= or <?
    if( radar_params.packet_number <= radar_params.total_packets )
    {
        status = x4driver_read_frame_normalized( x4driver, &frame_counter, radar_params.write_buffer + frame_index*fdata_count, fdata_count );
        if (status != XEP_ERROR_X4DRIVER_OK)
        {
            printf("Error getting x4 frame!!\n");
            printf("Error code=%d\n", status);
        }
    }
    else
    {
        //printf("Reached an extra call past end of data!\n");
        status = x4driver_read_frame_normalized( x4driver, &frame_counter, garbage_buffer, fdata_count );
        if (status != XEP_ERROR_X4DRIVER_OK)
        {
            printf("Error getting x4 frame!!\n");
            printf("Error code=%d\n", status);
        }
    }
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        // if 0 send all at end
        if (radar_params.chunk_size > 0){
            radar_params.current_chunk_count++;
            // send either when the chunk size is reached or when the packet number is the last one
            if (radar_params.current_chunk_count >= radar_params.chunk_size || radar_params.packet_number >= radar_params.total_packets){
                *radar_params.frameReady = 1 ;
                radar_params.current_chunk_count = 0;
                printf("return chunk radar_params.current_chunk_index: %d\n", radar_params.chunk_index);
                radar_params.chunk_index++;
            }
        }
        else if( radar_params.packet_number >= radar_params.total_packets )
        {
            *radar_params.frameReady = 1 ;
            printf("All frames done - sending all frames\n");
        }

        if(radar_params.packet_number >= radar_params.total_packets){
            radar_params.all_frames_ready = 1;
        }
    }
}

static uint32_t x4driver_callback_take_sem(void *sem, uint32_t timeout)
{
    x4driver_mutex.lock();
    return 1;
}

static void x4driver_callback_give_sem(void *sem)
{
    x4driver_mutex.unlock();
}

static uint32_t x4driver_callback_pin_set_enable(void *user_reference, uint8_t value)
{
    XepRadarX4DriverUserReference_t *x4driver_user_reference = (XepRadarX4DriverUserReference_t *)user_reference;
    int status = radar_hal_pin_set_enable(x4driver_user_reference->radar_handle, value);
    return status;
}

static uint32_t x4driver_callback_spi_write(void *user_reference, uint8_t *data, uint32_t length)
{
    XepRadarX4DriverUserReference_t *x4driver_user_reference = (XepRadarX4DriverUserReference_t *)user_reference;
    return radar_hal_spi_write(x4driver_user_reference->radar_handle, data, length);
}
static uint32_t x4driver_callback_spi_read(void *user_reference, uint8_t *data, uint32_t length)
{
    XepRadarX4DriverUserReference_t *x4driver_user_reference = (XepRadarX4DriverUserReference_t *)user_reference;
    return radar_hal_spi_read(x4driver_user_reference->radar_handle, data, length);
}

static uint32_t x4driver_callback_spi_write_read(void *user_reference, uint8_t *wdata, uint32_t wlength, uint8_t *rdata, uint32_t rlength)
{
    XepRadarX4DriverUserReference_t *x4driver_user_reference = (XepRadarX4DriverUserReference_t *)user_reference;
    return radar_hal_spi_write_read(x4driver_user_reference->radar_handle, wdata, wlength, rdata, rlength);
}

static void x4driver_callback_wait_us(uint32_t us)
{
    delayMicroseconds(us);
}

// TODO: here there is a change from the novelda code - check the docs to see the options
void x4driver_enable_ISR(void *user_reference, uint32_t enable)
{
    if (enable == 1)
    {
        pinMode(X4_GPIO_INT, PINMODE_INPUT);
//        pullUpDnControl(X4_GPIO_INT, PUD_DOWN);
        if (wiringXxISR(X4_GPIO_INT, ISR_MODE_RISING, &x4driver_data_ready) < 0)
//        if (wiringXxISR(X4_GPIO_INT, ISR_MODE_FALLING, &x4driver_data_ready) < 0)
        {
            printf("unable to setup ISR");
        }
    }
    else
        pinMode(X4_GPIO_INT, PINMODE_OUTPUT); //disable Interrupt
}

uint32_t task_radar_init(X4Driver_t **x4driver)
{
    XepRadarX4DriverUserReference_t *x4driver_user_reference = (XepRadarX4DriverUserReference_t *)malloc(sizeof(XepRadarX4DriverUserReference_t));
    memset(x4driver_user_reference, 0, sizeof(XepRadarX4DriverUserReference_t));

    void *radar_hal_memory =  malloc(radar_hal_get_instance_size());

    int status = radar_hal_init(&(x4driver_user_reference->radar_handle), radar_hal_memory);

//	strcpy(userLogPrefix, "ronen");
	sprintf(logFileName, "%sNovelda_Scans.csv", "ronen");
	printf("\nLogging to file %s\n\n", logFileName);
	fpFile = fopen(logFileName, "w+");
	if ( !fpFile )
	{
		printf("Error opening %s\n", logFileName);

	}
	sprintf(logFileName2, "%sNovelda_ScansHex.csv", "ronen");
	fpFile2 = fopen(logFileName2, "w+");
	if ( !fpFile2 )
	{
		printf("Error opening %s\n", logFileName);

	}

	printf("ronen aharoni 1\n") ;
#if 1

#ifdef DEBUG
    if (status == XT_SUCCESS)
    {
        printf("radar_hal_init success\n");
    }
    else
    {
        printf("radar_hal_init unknown situation\n");
    }
#endif // DEBUG

    //! [X4Driver Platform Dependencies]

    // X4Driver lock mechanism, including methods for locking and unlocking.
    X4DriverLock_t lock;
    lock.object = (void *)&x4driver_mutex;
    lock.lock = x4driver_callback_take_sem;
    lock.unlock = x4driver_callback_give_sem;

    // X4Driver timer for generating sweep FPS on MCU. Not used when sweep FPS is generated on X4.
    //    uint32_t timer_id_sweep = 2;
    X4DriverTimer_t timer_sweep;
    //    timer_sweep.object = xTimerCreate("X4Driver_sweep_timer", 1000 / portTICK_PERIOD_MS, pdTRUE, (void*)timer_id_sweep, x4driver_timer_sweep_timeout);
    //    timer_sweep.configure = x4driver_timer_set_timer_timeout_frequency;

    // X4Driver timer used for driver action timeout.
    //    uint32_t timer_id_action = 3;
    X4DriverTimer_t timer_action;
    //    timer_action.object = xTimerCreate("X4Driver_action_timer", 1000 / portTICK_PERIOD_MS, pdTRUE, (void*)timer_id_action, x4driver_timer_action_timeout);
    //	timer_action.configure = x4driver_timer_set_timer_timeout_frequency;

    // X4Driver callback methods.
    X4DriverCallbacks_t x4driver_callbacks;

    x4driver_callbacks.pin_set_enable = x4driver_callback_pin_set_enable; // X4 ENABLE pin
    x4driver_callbacks.spi_read = x4driver_callback_spi_read;             // SPI read method
    x4driver_callbacks.spi_write = x4driver_callback_spi_write;           // SPI write method
    x4driver_callbacks.spi_write_read = x4driver_callback_spi_write_read; // SPI write and read method
    x4driver_callbacks.wait_us = x4driver_callback_wait_us;               // Delay method
                                                                          //  x4driver_callbacks.notify_data_ready = x4driver_notify_data_ready;      // Notification when radar data is ready to read
                                                                          //  x4driver_callbacks.trigger_sweep = x4driver_trigger_sweep_pin;          // Method to set X4 sweep trigger pin
    x4driver_callbacks.enable_data_ready_isr = x4driver_enable_ISR;       // Control data ready notification ISR

    void *x4driver_instance_memory = malloc(x4driver_get_instance_size()); //pvPortMalloc(x4driver_get_instance_size());
    //x4driver_create(x4driver, x4driver_instance_memory, &x4driver_callbacks,&lock,&timer_sweep,&timer_action, (void*)x4driver_user_reference);
    x4driver_create(x4driver, x4driver_instance_memory, &x4driver_callbacks, &lock, &timer_sweep, &timer_action, x4driver_user_reference);

#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_create success\n");
    }
    else
    {
        printf("x4driver_create unknow situcation\n");
    }
#endif // DEBUG

    // Nothing is done with this..
    RadarTaskParameters_t *task_parameters = (RadarTaskParameters_t *)malloc(sizeof(RadarTaskParameters_t));
    //task_parameters->dispatch = dispatch;
    task_parameters->x4driver = *x4driver;
    // TODO: what is this? Is it novelda specific or SOM (check github on raspberry pi)
    task_parameters->x4driver->spi_buffer_size = 192 * 32;
    task_parameters->x4driver->spi_buffer = (uint8_t *)malloc(task_parameters->x4driver->spi_buffer_size);
/*
    if( wiringXSPIGetSharedMemory( task_parameters->x4driver->spi_buffer ) == ERR )
    {
    	printf("ERR: fail to allocate shared memory \n");
    	task_parameters->x4driver->spi_buffer = (uint8_t *)malloc(task_parameters->x4driver->spi_buffer_size);
    }
*/
    if ((((uintptr_t)task_parameters->x4driver->spi_buffer) % 32) != 0)
    {
        int alignment_diff = 32 - (((uintptr_t)task_parameters->x4driver->spi_buffer) % 32);
        task_parameters->x4driver->spi_buffer += alignment_diff;
        task_parameters->x4driver->spi_buffer_size -= alignment_diff;
    }
    task_parameters->x4driver->spi_buffer_size -= task_parameters->x4driver->spi_buffer_size % 32;

    //    xTaskCreate(task_radar, (const char * const) "Radar", TASK_RADAR_STACK_SIZE, (void*)task_parameters, TASK_RADAR_PRIORITY, &h_task_radar);
    //    x4driver_user_reference->radar_task_handle = h_task_radar;

    // TODO: downconversion bug
    //task_parameters->x4driver->downconversion_enabled=1;
#endif
    return XT_SUCCESS;
}
int task_state = 1 ;
uint32_t taskRadar( void* args )
{
    // NoveldaConfigureRadar in production code
	uint8_t  prf_div = 0 ;
#if !DEMO
	float fps = 0 ;
#endif
    uint32_t status = 0;
    //uint8_t* data_frame;


    //initialize radar task
    printf("task_radar start!\n");
    status = task_radar_init(&x4driver);

    TaskRadarGpioInit();
//    TaskRadarSetAntenna( 1 );
    TaskRadarResetRadar();

#ifdef DEBUG
    if (status == XT_SUCCESS)
    {
        printf("task_radar_init success\n");
    }
    else if (status == XT_ERROR)
    {
        printf("task_radar_init failure\n");
    }
    else
    {
        printf("task_radar_init unknown situation\n");
    }
#endif // DEBUG

    xtx4driver_errors_t tmp_status = (xtx4driver_errors_t)x4driver_init(x4driver);
#if 1

#ifdef DEBUG
    if (tmp_status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_init success\n");
    }
    else
    {
        printf("x4driver_init unknown situation\n");
    }
#endif // DEBUG

    status = x4driver_set_sweep_trigger_control(x4driver, SWEEP_TRIGGER_X4); // By default let sweep trigger control done by X4
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_set_sweep_trigger_control success\n");
    }
    else
    {
        printf("x4driver_set_sweep_trigger_control unknown situation\n");
    }
#endif // DEBUG

    //    x4_initialize_status = tmp_status;

    status = x4driver_set_dac_min(x4driver, 949);
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error setting dac minimum\n");
        printf("Error code=%d\n", status);
#endif
//        return 1;
        return status;
    }
#ifdef DEBUG
    printf("x4driver_set_dac_min success\n");
#endif
    status = x4driver_set_dac_max(x4driver, 1100);
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error setting dac maximum\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return status;
    }
#ifdef DEBUG
    printf("x4driver_set_dac_max success\n");
#endif
    status = x4driver_set_iterations(x4driver, 64);
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_iterations\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return status;
    }
#ifdef DEBUG
    printf("x4driver_set_iterations success\n");
#endif
    status = x4driver_set_pulses_per_step(x4driver, 5);
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_pulses_per_step\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return status;
    }
#ifdef DEBUG
    printf("x4driver_set_pulses_per_step success\n");
#endif
    status = x4driver_set_downconversion(x4driver, 0); // Radar data as downconverted baseband IQ, not RF.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_downconversion\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return status;
    }
#ifdef DEBUG
    printf("x4driver_set_downconversion success\n");
#endif

    status = x4driver_set_frame_area_offset(x4driver, 0); // Given by module HW. Makes frame_area start = 0 at front of module.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area_offseto\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return status;
    }
#ifdef DEBUG
    printf("x4driver_set_frame_area_offset success\n");
#endif

    status = x4driver_set_frame_area(x4driver, 1.5, 4.0 ); // Observe from 0.5m to 4.0m.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return status;
    }
    printf("x4driver_set_frame_area success\n");

    status = x4driver_check_configuration(x4driver);
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_check_configuration success\n");
    }
    else
    {
        printf("x4driver_check_configuration unknown situation. Status %d\n", status);
    }
#endif // DEBUG

    x4driver_get_prf_div( x4driver, &prf_div);
    printf("prf is %d \n" , prf_div );

    RadarChangeTxFreq( TX_CENTER_FREQUENCY_KCC_8_748GHz ) ;

    // this section for debug only
    // TODO: I don't think this is necessary, but more importantly I want to check
    // radar.state = INIT_STATE ;
    // status = x4driver_set_fps(x4driver, 5); // Generate 5 frames per second
    // if (status != XEP_ERROR_X4DRIVER_OK)
    // {
    // #ifdef DEBUG
    // 	printf("Error in x4driver_set_fps\n");
    // 	printf("Error code=%d\n", status);
    // #endif
    // 	return (void*)1;
    // }
    printf("End of Debug section\n");
    return status;
    // end of debug section
#if !DEMO
    /***************set fps, this will trigger data output***************/
    status = x4driver_set_fps(x4driver, 250); // Generate 5 frames per second
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_fps\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return (void*)1;
    }
#ifdef DEBUG
    printf("x4driver_set_fps success\n");
#endif

    x4driver_get_fps(x4driver, &fps);
    printf("ronen fps is %f \n" , fps );

#endif

#endif

    // for (;;)
    // {
    // }

}

uint8_t RadarChangeRange( float Rstart )
{
	uint32_t status = 0;
	float Rstop  = Rstart + 2.5 ;
    status = x4driver_set_frame_area(x4driver, Rstart, Rstop ); // Observe from 0.5m to 4.0m.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area\n");
        printf("Error code=%d\n", status);
#endif
        //
        return 1;
    }
    return 0 ;
}

// TODO - want to see if there is data coming in the callback if the radar is stopped - is this
// and official way to do it?
uint32_t StopRadar()
{
    uint32_t status = x4driver_set_fps(x4driver, 0); // Generate 5 frames per second
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_fps\n");
        printf("Error code=%d\n", status);
#endif
        return status;
    }
    return status;
}

uint8_t RadarStartStop( uint8_t mode )
{
	uint16_t pps = ( mode == RESPIRATION ) ? radar.params.ppsRespiration : radar.params.ppsMotion ;
	uint16_t  fps = ( mode == RESPIRATION ) ? radar.params.fpsRespiration : radar.params.fpsMotion ;

	uint32_t status = XEP_ERROR_X4DRIVER_OK ;

	switch( mode )
	{
		case STOP :
			pps = 0 ;
			fps = 0 ;
			break ;
		case MOTION:
			pps = radar.params.ppsMotion ;  // 5;//
			fps = radar.params.fpsMotion ; // 250 ; //
			printf("pps %d -- fps %d \n", pps , fps );
			break;
		case POSTURE :
			pps = radar.params.ppsPosture;
			fps = radar.params.fpsPosture ;
			printf("POSTURE pps %d -- fps %d \n", pps , fps );
			break ;
		case RESPIRATION :
			pps = radar.params.ppsRespiration ;
			fps = radar.params.fpsRespiration ;
			break ;
		default :
			break ;
	}


    /***************set fps, this will trigger data output***************/
    status = x4driver_set_fps(x4driver, fps ); // Generate 5 frames per second
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_fps\n");
        printf("Error code=%d\n", status);
#endif


    }

    if( mode == STOP )
    	return (uint8_t)status ;

	status = x4driver_set_pulses_per_step(x4driver, pps );
	if (status != XEP_ERROR_X4DRIVER_OK)
	{
#ifdef DEBUG
		printf("Error in x4driver_set_pulses_per_step\n");
		printf("Error code=%d\n", status);
#endif
	}

return (uint8_t)status;

}

uint8_t RadarChangeTxPower( xtx4_tx_power_t tx_power )
{
	x4driver_set_tx_power( x4driver,  tx_power) ;
	return 0 ;
}


uint8_t RadarChangeTxFreq( xtx4_tx_center_frequency_t tx_center_frequency )
{
	x4driver_set_tx_center_frequency( x4driver,  tx_center_frequency );
	return 0 ;
}

uint8_t RadarClearInterrupt( void )
{
	_x4driver_set_x4_sw_action(x4driver,11);
	return 0 ;
}

uint8_t RadarInterruptEnableDisable( uint8_t enable )
{
	// enable/disable interrupt callback by changing interrupt pin direction

//	x4driver_set_enable(x4driver,enable);
//	x4driver->callbacks.enable_data_ready_isr( x4driver->user_reference , enable );
	readFlag = enable ;
	return 0 ;
}

void* taskWriteToFile( void* args )
{
//	static uint16_t frameIndex = 0 ;
	static uint16_t packetCount = 0 ;
	uint8_t indx = 0 ;
	uint16_t imageIndex = 0  ;

//	char* buff = NULL ;

	while(1)
	{
		while(1)
		{
			if( WriteDataFlag )
			{
                uint16_t respSlice2 = radar.params.respTimeShort * radar.params.fpsRespiration ;
				indx = buffer_index ? 0 : 1 ;
				WriteDataFlag = false ;
				/*buff = */(char*)data_frame_normolized[indx] ;
				imageIndex = (radar.DataType == RESPIRATION) ? frameIndex % respSlice2 : frameIndex ;
				image[frameIndex][0] = imageIndex ;
                
				for (uint32_t i = 0; i < 389 ; i++)
				{
//					printf("shalom\n");
					//fprintf(fpFile, " %f\n", data_frame_normolized[indx][i]);
					image[imageIndex][i+1] =  data_frame_normolized[indx][i] ;
					//for( int j = 0 ; j < 4 ; j++ )
						//fprintf(fpFile2, ", %d", buff[i*4+j]);


				}
				//fprintf(fpFile, "\n");
#if DEMO
//				if( ( ( frameIndex ) == 0) || frameIndex > 197 )
//					printf("index = %d \n" , frameIndex );
#endif
				frameIndex++ ;
#if DEMO
				if( radar.state == INIT_STATE )
				{
					printf("error should not get here \n");
					pthread_mutex_lock(&SendFileMutex);
					packetIsReady = true ;
					pthread_mutex_unlock( &SendFileMutex );

				}
				else
				{
                    uint16_t respSlice = radar.params.respTimeShort * radar.params.fpsRespiration ;
					if( radar.DataType == RESPIRATION )
					{
						if( /*frameIndex &&*/  ( ( frameIndex   % respSlice ) == 0 ) )
						{
                            
							printf("no of packet is %d \n" , frameIndex );
							// in Resperation mode send data to remote side every 40 packet
							pthread_mutex_lock(&SendFileMutex);
                            printf("\n sendingNow value is: %s \n", sendingNow? "True":"False");
                            if (sendingNow) 
                            {
                                printf("\n ~~~~~~~~~ Sending packet over TCP and packet is full at the same time ~~~~~~~\n");
                            }
							packetIsReady = true ;
							pthread_mutex_unlock( &SendFileMutex );

							if( frameIndex >= radar.NoPacketToReceive )
							{
								fseek( fpFile , 0 , SEEK_SET );
								frameIndex = 0 ;

							}

						}
					}
					else
					{
						if( frameIndex >= radar.NoPacketToReceive  )
						{
		#else

						if( frameIndex >= 400 )
						{
							printf("finish sending \n" );
		#endif



		#if DEMO
							//RadarStartStop(0);
							pthread_mutex_lock(&SendFileMutex);
							packetIsReady = true ;
		//					printf("finish task writefile %d\n" ,frameIndex);

							pthread_mutex_unlock(&SendFileMutex);
		#endif
							fseek( fpFile , 0 , SEEK_SET );
							frameIndex = 0 ;
							break ;
						}
					}
				}

			}

		}

		if( packetCount > 15 )
			break ;
	}
	fclose(fpFile);
	fclose(fpFile2);

	printf("file closed \n");
	return (void*)1;
}

typedef struct
{
	int pinNoscl;
	int pinNo10  ;
	int PinNo67  ;
	int PinNo195 ; // not use
	int PinNo71  ;
	int PinNo72  ;
	int PinNo73  ;
}AntennaBaseLine;


AntennaBaseLine AntBaseLineArray[] =
{
		{ 1 , 1 ,	0 ,	0 ,	0 ,	0 ,	0 } ,
		{ 1 , 1 ,	0 ,	0 ,	0 ,	0 ,	1 } ,
		{ 1 , 1 ,	0 ,	0 ,	0 ,	1 ,	0 } ,
		{ 1 , 1 ,	0 ,	0 ,	0 ,	1 ,	1 } ,
		{ 1 , 1 ,	0 ,	0 ,	1 ,	0 ,	0 } ,
		{ 1 , 1 ,	0 ,	0 ,	1 ,	0 ,	1 } ,
		{ 1 , 1 ,	0 ,	0 ,	1 ,	1 ,	0 } ,
		{ 1 , 1 ,	0 ,	0 ,	1 ,	1 ,	1 } ,
		{ 1 , 0 ,	0 ,	1 ,	0 ,	0 ,	0 } ,
		{ 1 , 0 ,	0 ,	1 ,	0 ,	0 ,	1 } ,
		{ 1 , 0 ,	0 ,	1 ,	0 ,	1 ,	0 } ,
		{ 1 , 0 ,	0 ,	1 ,	0 ,	1 ,	1 } ,
		{ 1 , 0 ,	0 ,	1 ,	1 ,	0 ,	0 } ,
		{ 1 , 0 ,	0 ,	1 ,	1 ,	0 ,	1 } ,
		{ 1 , 0 ,	0 ,	1 ,	1 ,	1 ,	0 } ,
		{ 1 , 0 ,	0 ,	1 ,	1 ,	1 ,	1 } ,
		{ 0 , 1 ,	0 ,	0 ,	0 ,	0 ,	0 } ,
		{ 0 , 1 ,	0 ,	0 ,	0 ,	0 ,	1 } ,
		{ 0 , 1 ,	0 ,	0 ,	0 ,	1 ,	0 } ,
		{ 0 , 1 ,	0 ,	0 ,	0 ,	1 ,	1 } ,
		{ 0 , 1 ,	0 ,	0 ,	1 ,	0 ,	0 } ,
		{ 0 , 1 ,	0 ,	0 ,	1 ,	0 ,	1 } ,
		{ 0 , 1 ,	0 ,	0 ,	1 ,	1 ,	0 } ,
		{ 0 , 1 ,	0 ,	0 ,	1 ,	1 ,	1 } ,
		{ 0 , 0 ,	0 ,	1 ,	0 ,	0 ,	0 } ,
		{ 0 , 0 ,	0 ,	1 ,	0 ,	0 ,	1 } ,
		{ 0 , 0 ,	0 ,	1 ,	0 ,	1 ,	0 } ,
		{ 0 , 0 ,	0 ,	1 ,	0 ,	1 ,	1 } ,
		{ 0 , 0 ,	0 ,	1 ,	1 ,	0 ,	0 } ,
		{ 0 , 0 ,	0 ,	1 ,	1 ,	0 ,	1 } ,
		{ 0 , 0 ,	0 ,	1 ,	1 ,	1 ,	0 } ,
		{ 0 , 0 ,	0 ,	1 ,	1 ,	1 ,	1 } ,
		{ 1 , 1 ,	1 ,	0 ,	0 ,	0 ,	0 } ,
		{ 1 , 1 ,	1 ,	0 ,	0 ,	0 ,	1 } ,
		{ 1 , 1 ,	1 ,	0 ,	0 ,	1 ,	0 } ,
		{ 1 , 1 ,	1 ,	0 ,	0 ,	1 ,	1 } ,
		{ 1 , 1 ,	1 ,	0 ,	1 ,	0 ,	0 } ,
		{ 1 , 1 ,	1 ,	0 ,	1 ,	0 ,	1 } ,
		{ 1 , 1 ,	1 ,	0 ,	1 ,	1 ,	0 } ,
		{ 1 , 1 ,	1 ,	0 ,	1 ,	1 ,	1 } ,
		{ 1 , 0 ,	1 ,	1 ,	0 ,	0 ,	0 } ,
		{ 1 , 0 ,	1 ,	1 ,	0 ,	0 ,	1 } ,
		{ 1 , 0 ,	1 ,	1 ,	0 ,	1 ,	0 } ,
		{ 1 , 0 ,	1 ,	1 ,	0 ,	1 ,	1 } ,
		{ 1 , 0 ,	1 ,	1 ,	1 ,	0 ,	0 } ,
		{ 1 , 0 ,	1 ,	1 ,	1 ,	0 ,	1 } ,
		{ 1 , 0 ,	1 ,	1 ,	1 ,	1 ,	0 } ,
		{ 1 , 0 ,	1 ,	1 ,	1 ,	1 ,	1 } ,
		{ 0 , 1 ,	1 ,	0 ,	0 ,	0 ,	0 } ,
		{ 0 , 1 ,	1 ,	0 ,	0 ,	0 ,	1 } ,
		{ 0 , 1 ,	1 ,	0 ,	0 ,	1 ,	0 } ,
		{ 0 , 1 ,	1 ,	0 ,	0 ,	1 ,	1 } ,
		{ 0 , 1 ,	1 ,	0 ,	1 ,	0 ,	0 } ,
		{ 0 , 1 ,	1 ,	0 ,	1 ,	0 ,	1 } ,
		{ 0 , 1 ,	1 ,	0 ,	1 ,	1 ,	0 } ,
		{ 0 , 1 ,	1 ,	0 ,	1 ,	1 ,	1 } ,
		{ 0 , 0 ,	1 ,	1 ,	0 ,	0 ,	0 } ,
		{ 0 , 0 ,	1 ,	1 ,	0 ,	0 ,	1 } ,
		{ 0 , 0 ,	1 ,	1 ,	0 ,	1 ,	0 } ,
		{ 0 , 0 ,	1 ,	1 ,	0 ,	1 ,	1 } ,
		{ 0 , 0 ,	1 ,	1 ,	1 ,	0 ,	0 } ,
		{ 0 , 0 ,	1 ,	1 ,	1 ,	0 ,	1 } ,
		{ 0 , 0 ,	1 ,	1 ,	1 ,	1 ,	0 } ,
		{ 0 , 0 ,	1 ,	1 ,	1 ,	1 ,	1 } ,

};

typedef enum
{
	GPIO73  = 0 ,
	GPIO72  = 1 ,
	GPI071  = 2 ,
	GPIO10  = 3 ,
#if 0 // evaluation board
	SCL1 = 4 ,//9 , //
	GPIO195 = 5 ,//11 ,//
#else // real board
	SCL1 = 9 , //
	GPIO195 = 16 ,//11 ,not use
#endif
	GPIO67  = 6 ,
	GPIO_LED = 8 ,
	GPIO_ENABLE = 15 ,

}gpioNo;


void TaskRadarGpioInit( void )
{
    //GPIO initialization

	pinMode(GPIO73 , PINMODE_OUTPUT);
	pinMode(GPIO72 , PINMODE_OUTPUT);
	pinMode(GPI071 , PINMODE_OUTPUT);
	pinMode(GPIO10 , PINMODE_OUTPUT);
	pinMode(SCL1, PINMODE_OUTPUT);
    pinMode(GPIO195, PINMODE_OUTPUT);
	pinMode(GPIO67 , PINMODE_OUTPUT);
	pinMode( 8 , PINMODE_OUTPUT);
	pinMode( 15 , PINMODE_OUTPUT);
	digitalWrite( 15   , (digital_value_t)1  );


}

void TaskRadarResetRadar( void )
{
#define _1_SEC 1000000

	digitalWrite( GPIO_ENABLE   , (digital_value_t)LOW  );
	//delay()
	delayMicroseconds( _1_SEC );
	digitalWrite( GPIO_ENABLE   , (digital_value_t)HIGH  );

}

void TaskRadarSetAntenna( uint8_t AntennaBaseState )
{
	AntennaBaseLine* pAntBaseLine = NULL ;

	if( AntennaBaseState > 63 )
	{
		printf("invalid base Line antenna \n");
		return ;
	}

	pAntBaseLine = &AntBaseLineArray[ AntennaBaseState ];

	digitalWrite( GPIO73   , (digital_value_t) pAntBaseLine->PinNo73  );
	digitalWrite( GPIO72   , (digital_value_t) pAntBaseLine->PinNo72  );
	digitalWrite( GPI071   , (digital_value_t) pAntBaseLine->PinNo71  );
	digitalWrite( GPIO10   , (digital_value_t) pAntBaseLine->pinNo10  );
	digitalWrite( SCL1  , (digital_value_t) pAntBaseLine->pinNoscl );
//	digitalWrite( GPIO195  , (digital_value_t) pAntBaseLine->PinNo195 );
	digitalWrite( GPIO67   , (digital_value_t) pAntBaseLine->PinNo67  );

//	digitalWrite( GPIO73   , (digital_value_t) 0 );

}

char test[30] = { 0 } ;

int GetInputFromUser( int *argc, char *argv[] )
{
    char *line = NULL;
//    size_t len = 0;
	char * pch;
//	char* argv[20] = { 0 };
//	int argc = 0 ;
	int i = 0 ;
//	char c ;
	int index = 0 ;

	memset(test , 0 , 30 );
//	fflush(stdin);
//	getchar();
	puts("\nPlease enter the command line :");
	fgets( test , 30 , stdin );
	if( test )
	{
	//	getline(&line, &len, stdin);
		line = test ;
		printf("line %s \n" , line );

		argv[index++] = pch = strtok (line," ,-\n");

		while (pch != NULL)
		{
			argv[index++] = pch = strtok (NULL, " ,-\n");
		}

		index -= 1 ;
		if( index < 1 )
		{
			return -1 ;
		}
		for( i = 0 ; i < index ; i++)
		{
			printf("%d %s \n", i , argv[i]);
		}

		*argc = index ;
	//	free(line);

	//	fflush(stdin);

	#if DEBUGER
		getchar();
	#endif
	}
	return 0 ;
}

void printPinMux( int pin )
{
	nxpIMX6DQRMGetPinMux(pin);
}

int RadarConfigConstantParameters( RadarApiNoveldaParams *params )
{
	uint32_t status = 0;
	xtx4_tx_power_t tx_power = (xtx4_tx_power_t)0 ;
    status = x4driver_set_dac_min(x4driver, params->dacMin );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error setting dac minimum\n");
        printf("Error code=%d\n", status);
#endif
//        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_dac_min success\n");
#endif
    status = x4driver_set_dac_max(x4driver, params->dacMax);
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error setting dac maximum\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_dac_max success\n");
#endif
    status = x4driver_set_iterations(x4driver, params->iterations );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_iterations\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_iterations success\n");
#endif
    status = x4driver_set_frame_area_offset(x4driver, 0); // Given by module HW. Makes frame_area start = 0 at front of module.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area_offseto\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_frame_area_offset success\n");
#endif
    status = x4driver_check_configuration(x4driver);
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_check_configuration success\n");
    }
    else
    {
        printf("x4driver_check_configuration unknown situation. Status %d\n", status);
    }
#endif // DEBUG
    status = x4driver_set_tx_power( x4driver, (xtx4_tx_power_t)params->transmitGain );
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_set_tx_power success\n");
    }
    else
    {
        printf("x4driver_set_tx_power fail\n");
    }

#endif // DEBUG

    x4driver_set_tx_center_frequency( x4driver,  (xtx4_tx_center_frequency_t)params->novelda_freq );

#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_set_tx_center_frequency success\n");
    }
    else
    {
        printf("x4driver_set_tx_center_frequency fail\n");
    }

#endif // DEBUG

    status = x4driver_get_tx_power(  x4driver, &tx_power);

#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_get_tx_power success  %d \n" , tx_power );
    }
    else
    {
        printf("x4driver_set_tx_power fail\n");
    }
#endif
    // no? - why in the RadarConfig is status not returned?
	return status ;
}


int RadarConfigDynamicParameters( RadarApiNoveldaParams *params )
{
	uint32_t status = 0;
    status = x4driver_set_pulses_per_step(x4driver, params->pps );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_pulses_per_step\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_pulses_per_step success. Value: %d\n", params->pps);
#endif

    printf("prfDiv %d \n" , params->prfDiv );
    status = x4driver_set_prf_div( x4driver, params->prfDiv );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_prf_div\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
    /*
    Very strange - any time you set the prfDiv to less than 16 it violates
    that rx_mframes_coarse <= trx_clocks_per_pulse. Now I don't know how important it is but
    I don't think this condition is meant in the production code. I also don't fully understand
    the implication. You have a frame of a certain length - how is that related to prfDiv?
    It has something to do with the 12 samplers: 96*16 = 1536 = 12*16*8. 
    */
    status = x4driver_set_frame_length(x4driver, params->prfDiv);
    printf("set_frame_length status: %d\n", status);
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("set_frame_length success\n");
    }
    else
    {
        printf("set_frame_length failed. Status %d\n", status);
        return -1;
    }
#endif // DEBUG

    status = x4driver_set_downconversion(x4driver, params->downconversion_enable); // Radar data as downconverted baseband IQ, not RF.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_downconversion\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_downconversion success\n");
#endif
    printf("downconversion %d \n" , params->downconversion_enable );
    printf("Rstart_m %f, Rstop_m %f \n", params->Rstart_m, params->Rstart_m + params->startToStop_m); 
    
    // downconversion should be before setting frame area (determines bins)
    status = x4driver_set_frame_area(x4driver, params->Rstart_m , params->Rstart_m + params->startToStop_m ); // Observe from 0.5m to 4.0m.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
    
    printf("x4driver_set_frame_area success\n");
    uint32_t bin_length;
    x4driver_get_frame_bin_count(x4driver, &bin_length);
    printf("x4driver_get_frame_bin_count success. Value: %d\n", bin_length);
    radar_params.bins = bin_length;


    status = x4driver_check_configuration(x4driver);
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_check_configuration success\n");
    }
    else
    {
        printf("x4driver_check_configuration unknown situation. Status %d\n", status);
    }
#endif // DEBUG

}

int StartRadar(RadarApiNoveldaParams *params)
{
    uint32_t status = x4driver_set_fps(x4driver, params->fps); 
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_fps\n");
        printf("Error code=%d\n", status);
#endif
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_fps success\n");
#endif
    float32_t fps;
    x4driver_get_fps(x4driver, &fps);
    printf("fps is %f \n" , fps );
	return 0;
}

int RadarConfigParameters( NoveldaRadarParams *params )
{
	uint32_t status = 0;
	xtx4_tx_power_t tx_power = (xtx4_tx_power_t)0 ;
    status = x4driver_set_dac_min(x4driver, params->dacMin );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error setting dac minimum\n");
        printf("Error code=%d\n", status);
#endif
//        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_dac_min success\n");
#endif
    status = x4driver_set_dac_max(x4driver, params->dacMax);
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error setting dac maximum\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_dac_max success\n");
#endif
    status = x4driver_set_iterations(x4driver, params->iterationsMotion );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_iterations\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
                return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_iterations success\n");
#endif
    status = x4driver_set_pulses_per_step(x4driver, params->ppsMotion );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_pulses_per_step\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_pulses_per_step success\n");
#endif
    status = x4driver_set_downconversion(x4driver, 0); // Radar data as downconverted baseband IQ, not RF.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_downconversion\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_downconversion success\n");
#endif

    status = x4driver_set_frame_area_offset(x4driver, 0); // Given by module HW. Makes frame_area start = 0 at front of module.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area_offseto\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
#ifdef DEBUG
    printf("x4driver_set_frame_area_offset success\n");
#endif
    // TODO add tzero_ns
    status = x4driver_set_frame_area(x4driver, params->Rstart_m , params->Rstart_m + params->startToStop_m ); // Observe from 0.5m to 4.0m.
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_frame_area\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }
    printf("x4driver_set_frame_area success\n");

    printf("prfDiv %d \n" , params->prfDivMotion );
    status = x4driver_set_prf_div( x4driver, params->prfDivMotion );
    if (status != XEP_ERROR_X4DRIVER_OK)
    {
#ifdef DEBUG
        printf("Error in x4driver_set_prf_div\n");
        printf("Error code=%d\n", status);
#endif
        //        return 1;
        return -1;
    }

    status = x4driver_check_configuration(x4driver);
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_check_configuration success\n");
    }
    else
    {
        printf("x4driver_check_configuration unknown situation. Status: %d\n", status);
    }
#endif // DEBUG

    status = x4driver_set_tx_power( x4driver, (xtx4_tx_power_t)params->transmitGain );
#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_set_tx_power success\n");
    }
    else
    {
        printf("x4driver_set_tx_power fail\n");
    }

#endif // DEBUG


    status = x4driver_get_tx_power(  x4driver, &tx_power);

#ifdef DEBUG
    if (status == XEP_ERROR_X4DRIVER_OK)
    {
        printf("x4driver_get_tx_power success  %d \n" , tx_power );
    }
    else
    {
        printf("x4driver_set_tx_power fail\n");
    }
#endif

	return 0 ;
}

void RadarInitIndex( void )
{
	radar.NoPacketToReceive = 0 ;
	frameIndex = 0 ;
}
