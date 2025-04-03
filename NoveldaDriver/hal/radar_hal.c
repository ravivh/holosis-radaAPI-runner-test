#include "radar_hal.h"
#include "xep_hal.h"
#include <string.h>
#include "wiringx.h"
//#include <wiringPi.h>
//#include <wiringPiSPI.h>


int radar_hal_init(radar_handle_t ** radar_handle, void* instance_memory)
{
    int status = XT_SUCCESS;
    radar_handle_t *radar_handle_local = (radar_handle_t *) instance_memory;
    memset(radar_handle_local, 0, sizeof(radar_handle_t));
    radar_handle_local->radar_id = 0;
	//status = xtio_spi_init(
		//(void *)&(radar_handle_local->spi_handle),
		//(void *)((int)instance_memory + sizeof(radar_handle_t)),
		//NULL,
		//&(radar_handle_local->radar_id),
		//XTIO_SPI);
    *radar_handle = radar_handle_local;
    //wiringPiSetup();
	
    //if(wiringXSetup("hummingboard_base_dq", NULL) == -1) {
    if (wiringXSetup("hummingboard_pro", NULL) == -1) {
        wiringXGC();
		return -1;
	}

    // GPIO initionalization
    pinMode(X4_TEST_PIN, PINMODE_OUTPUT);
    pinMode(X4_ENABLE_PIN, PINMODE_OUTPUT);
    //pinMode(X4_GPIO_INT, PINMODE_INPUT);

    //pullUpDnControl (X4_GPIO_INT, PUD_DOWN );  // need to convert it to Imx Ronen

    //SPI initionalization
    //wiringPiSPISetup( SPI_CHANNEL, 500000 );

    // ronen comment it
    //wiringXSPISetup( SPI_CHANNEL, 25000000 );
    wiringXSPISetup(SPI_CHANNEL, SPI_CLK);
    //wiringXSPISetup(SPI_CHANNEL, 50000000);

    //xtio_set_direction(XTIO_X4_ENABLE, XTIO_OUTPUT, XTIO_PIN_LEVEL_LOW);
    //xtio_set_pin_mode(XTIO_X4_IO1, XTIO_PULL_DOWN);
    //xtio_set_pin_mode(XTIO_X4_IO2, XTIO_PULL_DOWN);

    return status;
}

int radar_hal_get_instance_size(void)
{
    int size = sizeof(radar_handle_t);
    //size += xtio_spi_get_instance_size();

    return size;
}

uint32_t radar_hal_pin_set_enable(radar_handle_t * radar_handle, uint8_t value)
{
    if (value == 1) {
        digitalWrite(X4_ENABLE_PIN, HIGH);
    } else if (value == 0) {
        digitalWrite(X4_ENABLE_PIN, LOW);
    } else {
        return XT_ERROR;
    }

    return XT_SUCCESS;
}

uint32_t radar_hal_spi_write_read(radar_handle_t * radar_handle, uint8_t* wdata,
                uint32_t wlength, uint8_t* rdata, uint32_t rlength)
{
    if ((0 == wlength) && (0 == rlength)) {
        return XT_SUCCESS;
    }
    if ((NULL == wdata) || (NULL == rdata)) {
        return XT_ERROR;
    }

    *(rdata - 1) = *wdata;
    //wiringPiSPIDataRW(SPI_CHANNEL,rdata-1,rlength+1);
    wiringXSPIDataRW(SPI_CHANNEL, rdata - 1, rlength + 1);

    return XT_SUCCESS;
}

uint32_t radar_hal_spi_write(radar_handle_t * radar_handle, uint8_t* data, uint32_t length)
{
    if (0 == length) {
        return XT_SUCCESS;
    }
    if (NULL == data) {
        return XT_ERROR;
    }

    //xtio_spi_handle_t * spi_handle = (xtio_spi_handle_t *)radar_handle->spi_handle;
    //return spi_handle->spi_write_read(spi_handle, data, length, NULL, 0);
    //wiringPiSPIDataRW(SPI_CHANNEL,data,length);
    wiringXSPIDataRW(SPI_CHANNEL, data, length);
 
    return XT_SUCCESS;
}

uint32_t radar_hal_spi_read(radar_handle_t * radar_handle, uint8_t* data, uint32_t length)
{
    if (0 == length) {
        return XT_SUCCESS;
    }
    if (NULL == data) {
        return XT_ERROR;
    }

    //xtio_spi_handle_t * spi_handle = (xtio_spi_handle_t *)radar_handle->spi_handle;
    //return spi_handle->spi_write_read(spi_handle, NULL, 0, data, length);

    return radar_hal_spi_write_read(radar_handle,NULL, 0, data, length);
}
