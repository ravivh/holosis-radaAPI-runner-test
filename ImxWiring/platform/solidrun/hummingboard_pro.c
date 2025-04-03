/*
	Copyright (c) 2016 CurlyMo <curlymoo1@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "soc.h"
#include "wiringx.h"
#include "platform.h"
#include "hummingboard_pro.h"

struct platform_t *hummingboardPro = NULL;


/**
 * linux-gpio-number = (gpio_bank - 1) * 32 + gpio_bit
 */
static int map[] = {
    122, /* UARTS3_RTS  -  J10.2   - GPIO5_IO09, index=0 */
    119, /* UARTS3_RXD  -  J10.3   - GPIO5_IO06, index=1 */
    120, /* UARTS3_TXD  -  J10.4   - GPIO5_IO07, index=2 */
    121, /* UARTS3_CTS  -  J10.1   - GPIO5_IO08, index=3 */
    117, /* Sec_TR_SSB  -  CON4.6  - GPIO5_IO04, index=4 */
    76,  /* AntRxi_SSB  -  CON4.14 - GPIO3_IO25, index=5 */
    74,  /* Sec_TX_LSB  -  CON4.33 - GPIO3_IO23, index=6 */
    141, /* GPIO1(R150) -  CON4    - GPIO5_IO28, index=7 */
    111, /* X4_ENABLE_PIN- J8.2    - GPIO4_IO30, index=8 */
};
/**
 * Pins that do not support interrupt,
 * have -1 as indicator, on the appropriate place
 */
static int irq[] = {
    122,    /* 122 <- GPIO5_IO09 <- UARTS3_RTS */    
    119,    /* 119 <- GPIO5_IO06 <- UARTS3_RXD */
    120,    /* 120 <- GPIO5_IO07 <- UARTS3_TXD */    
    121,    /* 121 <- GPIO5_IO08 <- UARTS3_CTS */    
    117,    /* 117 <- GPIO5_IO04 <- Sec_TR_SSB */    
    76,     /* 76  <- GPIO3_IO25 <- AntRxi_SSB */ 
    74,     /* 74  <- GPIO3_IO23 <- Sec_TX_LSB */ 
    141,    /* 141 <- GPIO5_IO28 <- GPIO1(R150)*/
    111     /* 111 <- GPIO4_IO30 <- X4_ENABLE_PIN*/
};


static int hummingboardProValidGPIO(int pin)
{
	if (pin >= 0 && pin < (sizeof(map)/sizeof(map[0]))) {
		return 0;
	} else {
		return -1;
	}
}

static int hummingboardProISR(int i, enum isr_mode_t mode)
{
	if (irq[i] == -1) {
		wiringXLog(LOG_ERR, "The %s gpio %d cannot be used as interrupt",
            hummingboardPro->name[0], i);
		return -1;
	}
	
    return hummingboardPro->soc->isr(i, mode);
}

static int hummingboardProSetup(void)
{
	hummingboardPro->soc->setup();
	hummingboardPro->soc->setMap(map, sizeof(map) / sizeof(map[0]));
	hummingboardPro->soc->setIRQ(irq, sizeof(irq) / sizeof(irq[0]));

	return 0;
}

void hummingboardProInit(void)
{
	platform_register(&hummingboardPro, "hummingboard_pro");
	platform_add_alias(&hummingboardPro, "hummingboard_pro_alias");

	hummingboardPro->soc = soc_get("NXP", "IMX8MP");

	hummingboardPro->digitalRead = hummingboardPro->soc->digitalRead;
	hummingboardPro->digitalWrite = hummingboardPro->soc->digitalWrite;
	hummingboardPro->pinMode = hummingboardPro->soc->pinMode;
	hummingboardPro->setup = &hummingboardProSetup;

	hummingboardPro->isr = &hummingboardProISR;
	hummingboardPro->waitForInterrupt = hummingboardPro->soc->waitForInterrupt;

	hummingboardPro->selectableFd = hummingboardPro->soc->selectableFd;
	hummingboardPro->gc = hummingboardPro->soc->gc;

	hummingboardPro->validGPIO = &hummingboardProValidGPIO;
}
