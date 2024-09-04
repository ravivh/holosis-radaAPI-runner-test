/*
	Copyright (c) 2016 CurlyMo <curlymoo1@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifndef __FreeBSD__
	#include <linux/spi/spidev.h>
	#include "i2c-dev.h"
#endif

#include "wiringx.h"
#include "taskRadar.h"
#include "radar_hal.h"

#include "imx6dqrm.h"
#include "imx6sdlrm.h"

#include "hummingboard_gate_edge_sdl.h"
#include "hummingboard_gate_edge_dq.h"
#include "hummingboard_base_pro_sdl.h"
#include "hummingboard_base_pro_dq.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#include "common.h"
//#include "../RadarHandler.h"

char *pSpiShareMemory = NULL;
pthread_t InterruptTid;

int wiringXSPIGetSharedMemory( uint8_t *pInput )
{
	if( pSpiShareMemory )
	{
		pInput = (uint8_t*)pSpiShareMemory ;
		return 0 ;
	}
	printf("Err:memory was not allocated \n");
	return -1 ;
}



void wiringXDefaultLog(int prio, char *file, int line, const char *format_str, ...);
static void *interruptHandler ( void *arg);

int wiringXxISR (int pin, int mode, void (*function)(void));

static struct platform_t *platform = NULL;
static int namenr = 0;
void (*_wiringXLog)(int, char *, int, const char *, ...) = wiringXDefaultLog;

static int issetup = 0;
static int isinit = 0;
static int spisetup = 0 ;
static volatile int    pinPass = -1 ;

static void (*isrFunctions )(void) ;

#ifndef __FreeBSD__
/* SPI Bus Parameters */

struct spi_t {
	uint8_t mode;
	uint8_t bits_per_word;
	uint16_t delay;
	uint32_t speed;
	int fd;
} spi_t;

static struct spi_t spi[2] = {
	{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};
#endif

#ifdef _WIN32
#define timeradd(a, b, result) \
	do { \
		(result)->tv_sec = (a)->tv_sec + (b)->tv_sec; \
		(result)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
		if ((result)->tv_usec >= 1000000L) { \
			++(result)->tv_sec; \
			(result)->tv_usec -= 1000000L; \
		} \
	} while (0)

#define timersub(a, b, result) \
	do { \
		(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
		(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
		if ((result)->tv_usec < 0) { \
			--(result)->tv_sec; \
			(result)->tv_usec += 1000000L; \
		} \
	} while (0)
#endif

/* Both the delayMicroseconds and the delayMicrosecondsHard
   are taken from wiringPi */
static void delayMicrosecondsHard(unsigned int howLong) {
	struct timeval tNow, tLong, tEnd ;

	gettimeofday(&tNow, NULL);
#ifdef _WIN32
	tLong.tv_sec  = howLong / 1000000;
	tLong.tv_usec = howLong % 1000000;
#else
	tLong.tv_sec  = (__time_t)howLong / 1000000;
	tLong.tv_usec = (__suseconds_t)howLong % 1000000;
#endif
	timeradd(&tNow, &tLong, &tEnd);

	while(timercmp(&tNow, &tEnd, <)) {
		gettimeofday(&tNow, NULL);
	}
}

EXPORT void delayMicroseconds(unsigned int howLong) {
	struct timespec sleeper;
#ifdef _WIN32
	long int uSecs = howLong % 1000000;
	unsigned int wSecs = howLong / 1000000;
#else
	long int uSecs = (__time_t)howLong % 1000000;
	unsigned int wSecs = howLong / 1000000;
#endif

	if(howLong == 0) {
		return;
	} else if(howLong  < 100) {
		delayMicrosecondsHard(howLong);
	} else {
#ifdef _WIN32
		sleeper.tv_sec = wSecs;
#else
		sleeper.tv_sec = (__time_t)wSecs;	
#endif
		sleeper.tv_nsec = (long)(uSecs * 1000L);
		nanosleep(&sleeper, NULL);
	}
}

void wiringXDefaultLog(int prio, char *file, int line, const char *format_str, ...) {
	va_list ap, apcpy;
	char buf[64], *l = malloc(128);
	int save_errno = -1, pos = 0, bytes = 0;

	if(l == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(-1);
	}

	save_errno = errno;

	memset(l, '\0', 128);
	memset(buf, '\0',  64);

	switch(prio) {
		case LOG_WARNING:
			pos += sprintf(l, "WARNING: ");
		break;
		case LOG_ERR:
			pos += sprintf(l, "ERROR: ");
		break;
		case LOG_INFO:
			pos += sprintf(l, "INFO: ");
		break;
		case LOG_NOTICE:
			pos += sprintf(l, "NOTICE: ");
		break;
		case LOG_DEBUG:
			pos += sprintf(l, "DEBUG: ");
		break;
		default:
		break;
	}

	va_copy(apcpy, ap);
	va_start(apcpy, format_str);
#ifdef _WIN32
	bytes = _vscprintf(format_str, apcpy);
#else
	bytes = vsnprintf(NULL, 0, format_str, apcpy);
#endif
	if(bytes == -1) {
		fprintf(stderr, "ERROR: unproperly formatted wiringX log message %s\n", format_str);
	} else {
		va_end(apcpy);
		if((l = realloc(l, (size_t)bytes+(size_t)pos+3)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(-1);
		}
		va_start(ap, format_str);
		pos += vsprintf(&l[pos], format_str, ap);
		va_end(ap);
	}
	l[pos++]='\n';
	l[pos++]='\0';

	fprintf(stderr, "%s", l);

	free(l);
	errno = save_errno;
}

static void wiringXInit(void) {
	if(isinit == 0) {
		printf("Ronen init wiringXInit \n");
		isinit = 1;
	} else {
		printf("Ronen system is already init \n");
		return;
	}

//	/* Init all SoC's */
//	allwinnerA10Init();
//	allwinnerA31sInit();
//	allwinnerH3Init();
//	allwinnerH5Init();
//	nxpIMX6DQRMInit();
//	nxpIMX6SDLRMInit();
//	broadcom2835Init();
//	broadcom2836Init();
//	amlogicS805Init();
//	amlogicS905Init();
//	exynos5422Init();
//
//	/* Init all platforms */
//	pcduino1Init();
//	bananapi1Init();
//	bananapiM2Init();
//	orangepipcpInit();
//	orangepipc2Init();
//	hummingboardBaseProSDLInit();
//	hummingboardBaseProDQInit();
//	hummingboardGateEdgeSDLInit();
//	hummingboardGateEdgeDQInit();
//	raspberrypi1b1Init();
//	raspberrypi1b2Init();
//	raspberrypi1bpInit();
//	raspberrypizeroInit();
//	raspberrypi2Init();
//	raspberrypi3Init();
//	odroidc1Init();
//	odroidc2Init();
//	odroidxu4Init();

	nxpIMX6DQRMInit();

	hummingboardBaseProDQInit();

}

EXPORT int wiringXSetup(char *name, void (*func)(int, char *, int, const char *, ...)) {
	if(issetup == 0) {
		printf("wiring setup  now \n");
		issetup = 1;
	} else {
		printf("wiring is already setup \n");
		return 0;
	}

	if(func != NULL) {
		_wiringXLog = func;
	} else {
		_wiringXLog = wiringXDefaultLog;
	}

	wiringXInit();

	if(name != NULL) {
		if((platform = platform_get_by_name(name, &namenr)) == NULL) {
			char *tmp = NULL;
			char message[1024];
			int l = 0;
			l = snprintf(message, 1023-l, "The %s is an unsupported or unknown platform\n", name);
			l += snprintf(&message[l], 1023-l, "\tsupported wiringX platforms are:\n");
			int i = 0;
			while((tmp = platform_iterate_name(i++)) != NULL) {
				l += snprintf(&message[l], 1023-l, "\t- %s\n", tmp);
			}
			wiringXLog(LOG_ERR, message);
			return -1;
		}
		platform->setup();
	}

	return 0;
}

EXPORT char *wiringXPlatform(void) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return NULL;
	}
	return platform->name[namenr];
}

EXPORT int pinMode(int pin, enum pinmode_t mode) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	} else if(platform->pinMode == NULL) {
		wiringXLog(LOG_ERR, "The %s does not support the pinMode functionality", platform->name[namenr]);
		return -1;
	}
	return platform->pinMode(pin, mode);
}

EXPORT int digitalWrite(int pin, enum digital_value_t value) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	}	else if(platform->digitalWrite == NULL) {
		wiringXLog(LOG_ERR, "The %s does not support the digitalWrite functionality", platform->name[namenr]);
		return -1;
	}
	return platform->digitalWrite(pin, value);
}

EXPORT int digitalRead(int pin) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	}	else if(platform->digitalRead == NULL) {
		wiringXLog(LOG_ERR, "The %s does not support the digitalRead functionality", platform->name[namenr]);
		return -1;
	}
	return platform->digitalRead(pin);
}

EXPORT int wiringXISR(int pin, enum isr_mode_t mode)
{
//	pthread_t threadId ;

	if(platform == NULL)
	{
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	}
	else if(platform->isr == NULL)
	{
		wiringXLog(LOG_ERR, "The %s does not support the wiringXISR functionality", platform->name[namenr]);
		return -1;
	}
/*
	//store the pin
	pinPass = pin ;
	// start thread to catch the interrupt
    pthread_create (&threadId, NULL, interruptHandler, NULL) ;
*/

	return platform->isr(pin, mode);
}

EXPORT int waitForInterrupt(int pin, int ms) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	}	else if(platform->waitForInterrupt == NULL) {
		wiringXLog(LOG_ERR, "The %s does not support the waitForInterrupt functionality", platform->name[namenr]);
		return -1;
	}

	return platform->waitForInterrupt(pin, ms);
}

EXPORT int wiringXValidGPIO(int pin) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	}	else if(platform->validGPIO == NULL) {
		wiringXLog(LOG_ERR, "The %s does not support the wiringXValidGPIO functionality", platform->name[namenr]);
		return -1;
	}
	return platform->validGPIO(pin);
}

#ifndef __FreeBSD__
#if 0
EXPORT int wiringXI2CRead(int fd) {
	return i2c_smbus_read_byte(fd);
}

EXPORT int wiringXI2CReadReg8(int fd, int reg) {
	return i2c_smbus_read_byte_data(fd, reg);
}

EXPORT int wiringXI2CReadReg16(int fd, int reg) {
	return i2c_smbus_read_word_data(fd, reg);
}

EXPORT int wiringXI2CWrite(int fd, int data) {
	return i2c_smbus_write_byte(fd, data);
}

EXPORT int wiringXI2CWriteReg8(int fd, int reg, int data) {
	return i2c_smbus_write_byte_data(fd, reg, data);
}

EXPORT int wiringXI2CWriteReg16(int fd, int reg, int data) {
	return i2c_smbus_write_word_data(fd, reg, data);
}
#endif

EXPORT int wiringXI2CSetup(const char *path, int devId) {
	int fd = 0;

	if((fd = open(path, O_RDWR)) < 0) {
		wiringXLog(LOG_ERR, "wiringX failed to open %s for reading and writing", path);
		return -1;
	}

	if(ioctl(fd, I2C_SLAVE, devId) < 0) {
		wiringXLog(LOG_ERR, "wiringX failed to set %s to slave mode", path);
		return -1;
	}

	return fd;
}

EXPORT int wiringXSPIGetFd(int channel) {
	return spi[channel & 1].fd;
}

EXPORT int wiringXSPIDataRW(int channel, unsigned char *data, int len) {
	struct spi_ioc_transfer tmp;
	memset(&tmp, 0, sizeof(tmp));
	channel &= 1;


	tmp.tx_buf = (uintptr_t)data;
	tmp.rx_buf = (uintptr_t)data;
	tmp.len = len;
	tmp.delay_usecs = spi[channel].delay;
	tmp.speed_hz = spi[channel].speed;
	tmp.bits_per_word = spi[channel].bits_per_word;
#ifdef SPI_IOC_WR_MODE32
	tmp.tx_nbits = 0;
#endif
#ifdef SPI_IOC_RD_MODE32
	tmp.rx_nbits = 0;
#endif
//	nxpIMX6DQRMPinInterruptMask( 7, (uint8_t)MASK ) ;
/*
	if( len > 2 )
	{
//		printf("len %d \n" , len );
		digitalWrite( X4_TEST_PIN, HIGH);
	}
*/
//	digitalWrite( X4_TEST_PIN, HIGH);
		if(ioctl(spi[channel].fd, SPI_IOC_MESSAGE(1), &tmp) < 0)
		{
			wiringXLog(LOG_ERR, "wiringX is unable to read/write from channel %d (%s)", channel, strerror(errno));
			return -1;
		}
//		memcpy( data , pSpiShareMemory , len );


//	digitalWrite( X4_TEST_PIN, LOW);

	return 0;
}

EXPORT int wiringXSPISetup(int channel, int speed) {

	const char *device = NULL;


	uint32_t speedDebug = 0 ;
	channel &= 1;

	if(channel == 0) {
		device = "/dev/spidev0.0";
	} else {
		device = "/dev/spidev1.0";
	}

	if( spisetup == 0 )
	{
		spisetup = 1 ;
		printf("open spi channel \n");
		if((spi[channel].fd = open(device, O_RDWR)) < 0) {
			wiringXLog(LOG_ERR, "wiringX is unable to open SPI device %s (%s)", device, strerror(errno));
			printf("fail to open spi channel %s (%s) \n" , device , strerror(errno));
			return -1;
		}
	}
	else
	{
		printf("spi channle is already open \n");
	}

/*
    if(spi[channel].fd >= 0) {
    	pSpiShareMemory = (char*)mmap(0,
                4096*2,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
				spi[channel].fd,
                0);
        printf("%s", pSpiShareMemory);
//        munmap(pSpiShareMemory, 4096*2);

    }
*/



	return spi[channel].fd;
	spi[channel].speed = speed;

	if(ioctl(spi[channel].fd, SPI_IOC_WR_MODE, &spi[channel].mode) < 0) {
		wiringXLog(LOG_ERR, "wiringX is unable to set write mode for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}

	if(ioctl(spi[channel].fd, SPI_IOC_RD_MODE, &spi[channel].mode) < 0) {
		wiringXLog(LOG_ERR, "wiringX is unable to set read mode for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}

	if(ioctl(spi[channel].fd, SPI_IOC_WR_BITS_PER_WORD, &spi[channel].bits_per_word) < 0) {
		wiringXLog(LOG_ERR, "wiringX is unable to set write bits_per_word for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}

	if(ioctl(spi[channel].fd, SPI_IOC_RD_BITS_PER_WORD, &spi[channel].bits_per_word) < 0) {
		wiringXLog(LOG_ERR, "wiringX is unable to set read bits_per_word for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}

	if(ioctl(spi[channel].fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi[channel].speed) < 0) {
		wiringXLog(LOG_ERR, "wiringX is unable to set write max_speed for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}

	if(ioctl(spi[channel].fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi[channel].speed) < 0) {
		wiringXLog(LOG_ERR, "wirignX is unable to set read max_speed for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}

	if(ioctl(spi[channel].fd, SPI_IOC_RD_MAX_SPEED_HZ, &speedDebug ) < 0) {
		wiringXLog(LOG_ERR, "wirignX is unable to set read max_speed for device %s (%s)", device, strerror(errno));
		close(spi[channel].fd);
		return -1;
	}
	else
		printf("Ronen Aharoni speed is %d \n", speedDebug );

	return spi[channel].fd;
}
#endif

EXPORT int wiringXSerialOpen(const char *device, struct wiringXSerial_t wiringXSerial) {
	struct termios options;
	speed_t myBaud;
	int status = 0, fd = 0;

	switch(wiringXSerial.baud) {
		case 50: myBaud = B50; break;
		case 75: myBaud = B75; break;
		case 110:	myBaud = B110; break;
		case 134:	myBaud = B134; break;
		case 150:	myBaud = B150; break;
		case 200:	myBaud = B200; break;
		case 300:	myBaud = B300; break;
		case 600:	myBaud = B600; break;
		case 1200: myBaud = B1200; break;
		case 1800: myBaud = B1800; break;
		case 2400: myBaud = B2400; break;
		case 4800: myBaud = B4800; break;
		case 9600: myBaud = B9600; break;
		case 19200: myBaud = B19200; break;
		case 38400: myBaud = B38400; break;
		case 57600: myBaud = B57600; break;
		case 115200: myBaud = B115200; break;
		case 230400: myBaud = B230400; break;
		default:
		return -1;
	}

	if((fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1) {
		return -1;
	}

	fcntl(fd, F_SETFL, O_RDWR);

	tcgetattr(fd, &options);

	cfmakeraw(&options);
	cfsetispeed(&options, myBaud);
	cfsetospeed(&options, myBaud);

	options.c_cflag |= (CLOCAL | CREAD);

	options.c_cflag &= ~CSIZE;

	switch(wiringXSerial.databits) {
		case 7:
			options.c_cflag |= CS7;
		break;
		case 8:
			options.c_cflag |= CS8;
		break;
		default:
			wiringXLog(LOG_ERR, "wiringX serial interface can not handle the %d data size", wiringXSerial.databits);
		return -1;
	}
	switch(wiringXSerial.parity) {
		case 'n':
		case 'N':/*no parity*/
			options.c_cflag &= ~PARENB;
			options.c_iflag &= ~INPCK;
		break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK; /* Disable parity checking */
		break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB; /* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;
		break;
		case 'S':
		case 's': /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
		break;
		default:
			wiringXLog(LOG_ERR, "wiringX serial interface can not handle the %d parity", wiringXSerial.parity);
		return -1;
	}
	switch(wiringXSerial.stopbits) {
		case 1:
			options.c_cflag &= ~CSTOPB;
		break;
		case 2:
			options.c_cflag |= CSTOPB;
		break;
		default:
			wiringXLog(LOG_ERR, "wiringX serial interface can not handle the %d stop bit", wiringXSerial.stopbits);
		return -1;
	}
	switch(wiringXSerial.flowcontrol) {
		case 'x':
		case 'X':
			options.c_iflag |= (IXON | IXOFF | IXANY);
		break;
		case 'n':
		case 'N':
			options.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
		default:
			wiringXLog(LOG_ERR, "wiringX serial interface can not handle the %d flowcontol", wiringXSerial.flowcontrol);
		return -1;
	}

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 150;

	tcflush(fd,TCIFLUSH);

	tcsetattr(fd, TCSANOW | TCSAFLUSH, &options);

	ioctl(fd, TIOCMGET, &status);

	status |= TIOCM_DTR;
	status |= TIOCM_RTS;

	ioctl(fd, TIOCMSET, &status);

	return fd;
}

EXPORT void wiringXSerialFlush(int fd) {
	if(fd > 0) {
		tcflush(fd, TCIOFLUSH);
	} else {
		wiringXLog(LOG_ERR, "wiringX serial interface has not been opened");
	}
}

EXPORT void wiringXSerialClose(int fd) {
	if(fd > 0) {
		close(fd);
	}
}

EXPORT void wiringXSerialPutChar(int fd, unsigned char c) {
	if(fd > 0) {
		int x = write(fd, &c, 1);
		if(x != 1) {
			wiringXLog(LOG_ERR, "wiringX failed to write to serial device");
		}
	} else {
		wiringXLog(LOG_ERR, "wiringX serial interface has not been opened");
	}
}

EXPORT void wiringXSerialPuts(int fd, const char *s) {
	if(fd > 0) {
		int x = write(fd, s, strlen(s));
		if(x != strlen(s)) {
			wiringXLog(LOG_ERR, "wiringX failed to write to serial device");
		}
	} else {
		wiringXLog(LOG_ERR, "wiringX serial interface has not been opened");
	}
}

EXPORT void wiringXSerialPrintf(int fd, const char *message, ...) {
	va_list argp;
	char buffer[1024];

	memset(&buffer, '\0', 1024);

	if(fd > 0) {
		va_start(argp, message);
		vsnprintf(buffer, 1023, message, argp);
		va_end(argp);

		wiringXSerialPuts(fd, buffer);
	} else {
		wiringXLog(LOG_ERR, "wiringX serial interface has not been opened");
	}
}

EXPORT int wiringXSerialDataAvail(int fd) {
	int result = 0;

	if(fd > 0) {
		if(ioctl(fd, FIONREAD, &result) == -1) {
			return -1;
		}
		return result;
	} else {
		wiringXLog(LOG_ERR, "wiringX serial interface has not been opened");
		return -1;
	}
}

EXPORT int wiringXSerialGetChar(int fd) {
	uint8_t x = 0;

	if(fd > 0) {
		if(read(fd, &x, 1) != 1) {
			return -1;
		}
		return ((int)x) & 0xFF;
	} else {
		wiringXLog(LOG_ERR, "wiringX serial interface has not been opened");
		return -1;
	}
}

EXPORT int wiringXSelectableFd(int gpio) {
	if(platform == NULL) {
		wiringXLog(LOG_ERR, "wiringX has not been properly setup (no platform has been selected)");
		return -1;
	}	else if(platform->selectableFd == NULL) {
		wiringXLog(LOG_ERR, "The %s does not support the wiringXSelectableFd functionality", platform->name[namenr]);
		return -1;
	}
	return platform->selectableFd(gpio);
}

EXPORT int wiringXGC(void) {
	if(platform != NULL) {
		platform->gc();
		platform = NULL;
	}
	platform_gc();
	soc_gc();
	issetup = 0;
	isinit = 0;
	return 0;
}

EXPORT int wiringXSupportedPlatforms(char ***out) {
	wiringXInit();
	char *tmp = NULL;
	int i = 0 ;
//	int x = 0;
	while((tmp = platform_iterate_name(i++)) != NULL);

	if((*out = malloc(sizeof(char *)*i)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(-1);
	}

	i = 0;
	while((tmp = platform_iterate_name(i++)) != NULL) {
		if(((*out)[i-1] = strdup(tmp)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(-1);
		}
	}
	return i-1;
}



int wiringXxISR (int pin, int mode, void (*function)(void))
{
  //pthread_t threadId = pthread_self();
  InterruptTid = pthread_self();
//  pthread_attr_t tattr;
//  pthread_t tid;
  int ret;

//  int newprio = 2;
  struct sched_param param;

  // We'll set the priority to the maximum.
  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  // Attempt to set thread real-time priority to the SCHED_FIFO policy


//  ret = pthread_setschedparam(threadId, SCHED_FIFO, &param);
  ret = pthread_setschedparam(InterruptTid, SCHED_FIFO, &param);
  if (ret != 0) {
      // Print the error
      printf("Unsuccessful in setting thread realtime prio\n");
      //return;
  }
#if 0

  /* initialized with default attributes */
  ret = pthread_attr_init (&tattr);

  /* safe to get existing scheduling param */
  ret = pthread_attr_getschedparam (&tattr, &param);

  /* set the priority; others are unchanged */
  param.sched_priority = newprio;

  /* setting the new scheduling param */
  ret = pthread_attr_setschedparam (&tattr, &param);
#endif
  /* with new priority specified */
 // ret = pthread_create (&tid, &tattr, func, NULL);
/*
  const char *modeS ;
  char fName   [64] ;
  char  pinS [8] ;
  pid_t pid ;
  int   count, i ;
  char  c ;
  int   bcmGpioPin ;

  if ((pin < 0) || (pin > 63))
    return wiringPiFailure (WPI_FATAL, "wiringPiISR: pin must be 0-63 (%d)\n", pin) ;

   if (wiringPiMode == WPI_MODE_UNINITIALISED)
    return wiringPiFailure (WPI_FATAL, "wiringPiISR: wiringPi has not been initialised. Unable to continue.\n") ;
  else if (wiringPiMode == WPI_MODE_PINS)
    bcmGpioPin = pinToGpio [pin] ;
  else if (wiringPiMode == WPI_MODE_PHYS)
    bcmGpioPin = physToGpio [pin] ;
  else
    bcmGpioPin = pin ;

// Now export the pin and set the right edge
//	We're going to use the gpio program to do this, so it assumes
//	a full installation of wiringPi. It's a bit 'clunky', but it
//	is a way that will work when we're running in "Sys" mode, as
//	a non-root user. (without sudo)

  if (mode != INT_EDGE_SETUP)
  {
     if (mode == INT_EDGE_FALLING)
      modeS = "falling" ;
    else if (mode == INT_EDGE_RISING)
      modeS = "rising" ;
    else
      modeS = "both" ;

    sprintf (pinS, "%d", bcmGpioPin) ;

    if ((pid = fork ()) < 0)	// Fail
      return wiringPiFailure (WPI_FATAL, "wiringPiISR: fork failed: %s\n", strerror (errno)) ;

    if (pid == 0)	// Child, exec
    {
       if (access ("/usr/local/bin/gpio", X_OK) == 0)
      {
	execl ("/usr/local/bin/gpio", "gpio", "edge", pinS, modeS, (char *)NULL) ;
	return wiringPiFailure (WPI_FATAL, "wiringPiISR: execl failed: %s\n", strerror (errno)) ;
      }
      else if (access ("/usr/bin/gpio", X_OK) == 0)
      {
	execl ("/usr/bin/gpio", "gpio", "edge", pinS, modeS, (char *)NULL) ;
	return wiringPiFailure (WPI_FATAL, "wiringPiISR: execl failed: %s\n", strerror (errno)) ;
      }
      else
	return wiringPiFailure (WPI_FATAL, "wiringPiISR: Can't find gpio program\n") ;
    }
    else		// Parent, wait
      wait (NULL) ;
  }

// Now pre-open the /sys/class node - but it may already be open if
//	we are in Sys mode...

  if (sysFds [bcmGpioPin] == -1)
  {
    sprintf (fName, "/sys/class/gpio/gpio%d/value", bcmGpioPin) ;
    if ((sysFds [bcmGpioPin] = open (fName, O_RDWR)) < 0)
      return wiringPiFailure (WPI_FATAL, "wiringPiISR: unable to open %s: %s\n", fName, strerror (errno)) ;
  }

// Clear any initial pending interrupt

  ioctl (sysFds [bcmGpioPin], FIONREAD, &count) ;
  for (i = 0 ; i < count ; ++i)
    read (sysFds [bcmGpioPin], &c, 1) ;
*/
	wiringXISR ( pin,  mode );


  	isrFunctions = function ;
	pinPass = pin ;

//  pthread_mutex_lock (&pinMutex) ;
//    pthread_create ( &threadId , NULL, interruptHandler, NULL) ;
    ret = pthread_create ( &InterruptTid , NULL, interruptHandler, NULL) ;
    if( ret != 0 )
    {
    	printf     ("-----------------Fail to create interrupt Thread %ld \n"    , InterruptTid );
    }
    else
    {
    	InterruptTid = (unsigned long int)-1 ;
    	printf     ("+++++++++++++++++Success to create interrupt Thread %ld \n" , InterruptTid );

    }

//    while (pinPass != -1)
//      delay (1) ;
//  pthread_mutex_unlock (&pinMutex) ;

  return 0 ;
}


/*
 * interruptHandler:
 *	This is a thread and gets started to wait for the interrupt we're
 *	hoping to catch. It will call the user-function when the interrupt
 *	fires.
 *********************************************************************************
 */
#include <sys/poll.h>

static void *interruptHandler ( void *arg )
{
  int myPin ;
//  int myPinFd = platform->selectableFd(pinPass);
//  int mask = 0 ;
//  pthread_mutex_t count_mutex;
//  (void)piHiPri (55) ;	// Only effective if we run as root

  myPin   = pinPass ;
  pinPass = -1 ;




  for (;;)
  {
//		digitalWrite( 0 , 1);

#if 1
	if (waitForInterrupt (myPin, -1) > 0)
	{


		//x4driver_data_ready();
//		pthread_mutex_lock(&count_mutex);
	    isrFunctions () ;
//	    pthread_mutex_unlock(&count_mutex);
	}
#else
#if 1
//	struct timeval tv;

	fd_set fds1;
	char c ;
	int x = 0 ;
    // basic select call setup
	FD_ZERO(&fds1);
//	FD_SET(0, &fds);
	FD_SET(myPinFd, &fds1);
	printf("fd %08X \n" , fds1);

	// Set up timeout
//	tv.tv_sec = timeoutMs / 1000;
//	tv.tv_usec = (timeoutMs * 1000) % 1000000;
	digitalWrite( 0 , 1);
	x = read(myPinFd, &c, 1);
	if(x != 1) {
		return -1;
	}
	lseek(myPinFd, 0, SEEK_SET);


	if ( (mask = select(myPinFd + 1, NULL, NULL , &fds1, NULL)) > 0)
	{
		printf("maks is %08X \n" , mask );
		digitalWrite( 0 , 0);
		//read(myPinFd, &c, 1);
//		x = read(myPinFd, &c, 1);
		lseek(myPinFd, 0, SEEK_SET);
		if(read(myPinFd, &c, 1)  != 1)
		{
		}
		else
		{
			if( c == '1' )
			{

			// copy packet into buffer
				isrFunctions () ;
			}

		}


	}
#else
	struct pollfd polls;
	int x = 0;
	char c = 0 ;
	polls.fd = myPinFd;
	polls.events = POLLPRI;

	x = read(myPinFd, &c, 1);
	if(x != 1) {
		return -1;
	}
	lseek(myPinFd, 0, SEEK_SET);
//	digitalWrite( 0, HIGH);
	x = poll(&polls, 1, -1);

	digitalWrite( 0 , 0);
	/* Don't react to signals */
	if(x == -1 && errno == EINTR) {
		x = 0;
	}
	else
	{
		// copy packet into buffer
		isrFunctions () ;

	}

#endif

#endif
  }

  return NULL ;
}

