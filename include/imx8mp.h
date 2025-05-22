/*
	Copyright (c) 2016 CurlyMo <curlymoo1@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef __WIRINGX_IMX8MP_H_
#define __WIRINGX_IMX8MP_H_

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "soc.h"
#include "wiringx.h"

#define GPIOD_CONSUMER "nxpIMX8MPISR"

extern struct soc_t *nxpIMX8MP;

void nxpIMX8MPInit(void);
extern int nxpIMX8MPPinInterruptMask( int i, uint8_t mode ) ;
int nxpIMX8MPGetPinMux(int i);

#endif
