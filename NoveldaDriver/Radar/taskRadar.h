#ifndef TASK_RADAR_H
#define TASK_RADAR_H

#ifdef __cplusplus
#include "x4driver.h"
#include "common.h"

#define _1_SEC 1000000

typedef struct RadarApiNoveldaParams
{
	float32_t    Rstart_m        ;        // 1 (null)
	float32_t    startToStop_m   ;        // 3.5 (null)
	uint8_t  prfDiv          ;                    // 11 (null)
	xtx4_tx_power_t  transmitGain    ;
	uint16_t pps              ;
	uint8_t  iterations      ;
	float32_t fps             ;
	uint16_t dacMin          ;
	uint16_t dacMax          ;
	float32_t	 tZeroNs         ;               // 17  (null)
    xtx4_tx_center_frequency_t  novelda_freq    ;
	uint8_t downconversion_enable;
}RadarApiNoveldaParams;

typedef struct RadarApiParams
{
	// Do I need this?
	uint8_t data_type;
	uint8_t stop_radar_flag;
    // TODO: what if it is downconverted?
	float32_t* write_buffer;
	// overall packet number
	uint16_t packet_number;
	// debug - packet numbers
    uint32_t total_packets;
    // antenna loop index
    uint16_t antenna_index;
	// number of loops of acquisition parameters
	uint16_t repetitions;
	// antennas to loop over
	uint16_t antenna_num;
	int* antennas;
	RadarApiNoveldaParams params;
	// this will determine if radar is initialized
	uint8_t state;
	// bins - here it varies based on start/stop - probably should calculate it in python to allocate
	// the correct number of bins
	uint32_t bins;
	// chunk size
	uint32_t chunk_size;
	uint32_t chunk_index;
	// current chunk count
	uint32_t current_chunk_count;
    // signals that data is done
    uint8_t *frameReady;
    // signals that all frames are ready
    uint8_t all_frames_ready;
}RadarApiParams;


extern int task_state  ;
void x4driver_data_ready(void);

/**
 * @brief Radar task.
 *
 * @return nothing
 */
uint32_t taskRadar( void* args );

uint8_t RadarStartStop( uint8_t mode );

uint8_t RadarChangeTxPower( xtx4_tx_power_t tx_power );

uint8_t RadarClearInterrupt( void ) ;

void* taskWriteToFile( void* args );

int GetInputFromUser( int *argc, char *argv[] );

void TaskRadarSetAntenna( uint8_t AntennaBaseState );

uint8_t RadarChangeTxFreq( xtx4_tx_center_frequency_t tx_center_frequency )  ;

void printPinMux( int pin ) ;


uint8_t RadarChangeRange( float Rstart ) ;

uint8_t RadarInterruptEnableDisable( uint8_t enable );

int RadarConfigParameters( NoveldaRadarParams *params ) ;
uint32_t StopRadar();

void RadarInitIndex( void );

int RadarConfigConstantParameters( RadarApiNoveldaParams *params );
int RadarConfigDynamicParameters( RadarApiNoveldaParams *params );
int StartRadar(RadarApiNoveldaParams *params);

/**
 * @brief Initialize Radar task.
 *
 * @return Status of execution
 */
uint32_t task_radar_init(X4Driver_t** x4driver);

/*
class appDebug_xep : public AbstractDebugOutput
{
public:
    appDebug_xep(XepDispatch_t *dispatch);

    virtual uint32_t send_byte_stream(uint32_t contentID, uint32_t info, uint8_t *data, uint32_t length);
    virtual void stop();

    // cpu
    virtual uint64_t getTick_hw() ;
    virtual uint64_t getTick_os() ;

    void setDebugLevel(uint8_t debug_level){debug_level_ = debug_level;}

private:
    uint8_t debug_level_;

    XepDispatch_t* dispatch_ = nullptr;
};
*/

#endif

#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

//EXPORT_C void* AppDebug_xep_new(XepDispatch_t *dispatch);
#endif // TASK_RADAR_H
