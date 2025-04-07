/*
	Copyright (c) 2016 CurlyMo <curlymoo1@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "imx8mp.h"

#include <unistd.h>

struct soc_t *nxpIMX8MP = NULL;

static struct gpiod_chip *chip;
static struct gpiod_line *line;


#define FUNC_GPIO		0x5
#define GPIO_GDIR_REG_OFFSET 0x4
#define GPIO_PSR_REG_OFFSET  0x8
#define GPIO_IMR_REG_OFFSET  0x14

#define GPIO_CHIP_1 "/dev/gpiochip0"
#define GPIO_CHIP_2 "/dev/gpiochip1"
#define GPIO_CHIP_3 "/dev/gpiochip2"
#define GPIO_CHIP_4 "/dev/gpiochip3"
#define GPIO_CHIP_5 "/dev/gpiochip4"


static struct layout_t {
	char *name;
	int addr;

    struct {
		unsigned long offset;
		unsigned long bit;
	} data;

    struct {
		unsigned long offset;
		unsigned long bit;
	} select;

    int support;
	enum pinmode_t mode;
	int fd;
} layout[] = {
    // GPIO1_IOxy  ->  xy = 00..29
	{ "GPIO1_IO00", 0, { 0x200000, 0x00 }, { 0x330014, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //0
    { "GPIO1_IO01", 0, { 0x200000, 0x01 }, { 0x330018, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //1
	{ "GPIO1_IO02", 0, { 0x200000, 0x02 }, { 0x33001C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //2
	{ "GPIO1_IO03", 0, { 0x200000, 0x03 }, { 0x330020, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //3
	{ "GPIO1_IO04", 0, { 0x200000, 0x04 }, { 0x330024, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //4
	{ "GPIO1_IO05", 0, { 0x200000, 0x05 }, { 0x330028, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //5
	{ "GPIO1_IO06", 0, { 0x200000, 0x06 }, { 0x33002C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //6
	{ "GPIO1_IO07", 0, { 0x200000, 0x07 }, { 0x330030, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //7
	{ "GPIO1_IO08", 0, { 0x200000, 0x08 }, { 0x330034, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //8
	{ "GPIO1_IO09", 0, { 0x200000, 0x09 }, { 0x330038, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //9
	{ "GPIO1_IO10", 0, { 0x200000, 0x0A }, { 0x33003C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //10
	{ "GPIO1_IO11", 0, { 0x200000, 0x0B }, { 0x330040, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //11
	{ "GPIO1_IO12", 0, { 0x200000, 0x0C }, { 0x330044, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //12
	{ "GPIO1_IO13", 0, { 0x200000, 0x0D }, { 0x330048, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //13
	{ "GPIO1_IO14", 0, { 0x200000, 0x0E }, { 0x33004C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //14
	{ "GPIO1_IO15", 0, { 0x200000, 0x0F }, { 0x330050, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //15
    { "GPIO1_IO16", 0, { 0x200000, 0x10 }, { 0x330054, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //16
    { "GPIO1_IO17", 0, { 0x200000, 0x11 }, { 0x330058, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //17
    { "GPIO1_IO18", 0, { 0x200000, 0x12 }, { 0x33005C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //18
    { "GPIO1_IO19", 0, { 0x200000, 0x13 }, { 0x330060, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //19
    { "GPIO1_IO20", 0, { 0x200000, 0x14 }, { 0x330064, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //20
    { "GPIO1_IO21", 0, { 0x200000, 0x15 }, { 0x330068, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //21
    { "GPIO1_IO22", 0, { 0x200000, 0x16 }, { 0x33006C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //22
    { "GPIO1_IO23", 0, { 0x200000, 0x17 }, { 0x330070, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //23
    { "GPIO1_IO24", 0, { 0x200000, 0x18 }, { 0x330074, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //24
    { "GPIO1_IO25", 0, { 0x200000, 0x19 }, { 0x330078, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //25
    { "GPIO1_IO26", 0, { 0x200000, 0x1A }, { 0x33007C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //26
    { "GPIO1_IO27", 0, { 0x200000, 0x1B }, { 0x330080, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //27
    { "GPIO1_IO28", 0, { 0x200000, 0x1C }, { 0x330084, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //28
    { "GPIO1_IO29", 0, { 0x200000, 0x1D }, { 0x330088, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //29


    // GPIO2_IOxy  ->  xy = 00..20
	{ "GPIO2_IO00", 0, { 0x210000, 0x00 }, { 0x33008C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //30
    { "GPIO2_IO01", 0, { 0x210000, 0x01 }, { 0x330090, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //31
    { "GPIO2_IO02", 0, { 0x210000, 0x02 }, { 0x330094, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //32
    { "GPIO2_IO03", 0, { 0x210000, 0x03 }, { 0x330098, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //33
    { "GPIO2_IO04", 0, { 0x210000, 0x04 }, { 0x33009C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //34
    { "GPIO2_IO05", 0, { 0x210000, 0x05 }, { 0x3300A0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //35
    { "GPIO2_IO06", 0, { 0x210000, 0x06 }, { 0x3300A4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //36
    { "GPIO2_IO07", 0, { 0x210000, 0x07 }, { 0x3300A8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //37
    { "GPIO2_IO08", 0, { 0x210000, 0x08 }, { 0x3300AC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //38
    { "GPIO2_IO09", 0, { 0x210000, 0x09 }, { 0x3300B0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //39
    { "GPIO2_IO10", 0, { 0x210000, 0x0A }, { 0x3300B4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //40
    { "GPIO2_IO11", 0, { 0x210000, 0x0B }, { 0x3300B8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //41
    { "GPIO2_IO12", 0, { 0x210000, 0x0C }, { 0x3300BC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //42
    { "GPIO2_IO13", 0, { 0x210000, 0x0D }, { 0x3300C0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //43
    { "GPIO2_IO14", 0, { 0x210000, 0x0E }, { 0x3300C4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //44
    { "GPIO2_IO15", 0, { 0x210000, 0x0F }, { 0x3300C8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //45
    { "GPIO2_IO16", 0, { 0x210000, 0x10 }, { 0x3300CC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //46
    { "GPIO2_IO17", 0, { 0x210000, 0x11 }, { 0x3300D0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //47
    { "GPIO2_IO18", 0, { 0x210000, 0x12 }, { 0x3300D4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //48
    { "GPIO2_IO19", 0, { 0x210000, 0x13 }, { 0x3300D8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //49
    { "GPIO2_IO20", 0, { 0x210000, 0x14 }, { 0x3300DC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //50


    // GPIO3_IOxy  ->  xy = 00..29
	{ "GPIO3_IO00", 0, { 0x220000, 0x00 }, { 0x3300E0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //51
    { "GPIO3_IO01", 0, { 0x220000, 0x01 }, { 0x3300E4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //52
    { "GPIO3_IO02", 0, { 0x220000, 0x02 }, { 0x3300E8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //53
    { "GPIO3_IO03", 0, { 0x220000, 0x03 }, { 0x3300EC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //54
    { "GPIO3_IO04", 0, { 0x220000, 0x04 }, { 0x3300F0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //55
    { "GPIO3_IO05", 0, { 0x220000, 0x05 }, { 0x3300F4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //56
    { "GPIO3_IO06", 0, { 0x220000, 0x06 }, { 0x3300F8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //57
    { "GPIO3_IO07", 0, { 0x220000, 0x07 }, { 0x3300FC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //58
    { "GPIO3_IO08", 0, { 0x220000, 0x08 }, { 0x330100, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //59
    { "GPIO3_IO09", 0, { 0x220000, 0x09 }, { 0x330104, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //60
    { "GPIO3_IO10", 0, { 0x220000, 0x0A }, { 0x330108, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //61
    { "GPIO3_IO11", 0, { 0x220000, 0x0B }, { 0x33010C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //62
    { "GPIO3_IO12", 0, { 0x220000, 0x0C }, { 0x330110, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //63
    { "GPIO3_IO13", 0, { 0x220000, 0x0D }, { 0x330114, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //64
    { "GPIO3_IO14", 0, { 0x220000, 0x0E }, { 0x330118, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //65
    { "GPIO3_IO15", 0, { 0x220000, 0x0F }, { 0x33011C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //66
    { "GPIO3_IO16", 0, { 0x220000, 0x10 }, { 0x330120, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //67
    { "GPIO3_IO17", 0, { 0x220000, 0x11 }, { 0x330124, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //68
    { "GPIO3_IO18", 0, { 0x220000, 0x12 }, { 0x330128, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //69
    { "GPIO3_IO19", 0, { 0x220000, 0x13 }, { 0x33012C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //70
    { "GPIO3_IO20", 0, { 0x220000, 0x14 }, { 0x330130, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //71
    { "GPIO3_IO21", 0, { 0x220000, 0x15 }, { 0x330134, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //72
    { "GPIO3_IO22", 0, { 0x220000, 0x16 }, { 0x330138, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //73
    { "GPIO3_IO23", 0, { 0x220000, 0x17 }, { 0x33013C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //74
    { "GPIO3_IO24", 0, { 0x220000, 0x18 }, { 0x330140, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //75
    { "GPIO3_IO25", 0, { 0x220000, 0x19 }, { 0x330144, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //76
                                          //order changed!
    { "GPIO3_IO26", 0, { 0x220000, 0x1A }, { 0x330240, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //77
    { "GPIO3_IO27", 0, { 0x220000, 0x1B }, { 0x330244, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //78
    { "GPIO3_IO28", 0, { 0x220000, 0x1C }, { 0x330248, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //79
    { "GPIO3_IO29", 0, { 0x220000, 0x1D }, { 0x33024C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //80


    // GPIO4_IOxy  ->  xy = 00..31
	{ "GPIO4_IO00", 0, { 0x230000, 0x00 }, { 0x330148, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //81
    { "GPIO4_IO01", 0, { 0x230000, 0x01 }, { 0x33014C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //82
    { "GPIO4_IO02", 0, { 0x230000, 0x02 }, { 0x330150, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //83
    { "GPIO4_IO03", 0, { 0x230000, 0x03 }, { 0x330154, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //84
    { "GPIO4_IO04", 0, { 0x230000, 0x04 }, { 0x330158, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //85
    { "GPIO4_IO05", 0, { 0x230000, 0x05 }, { 0x33015C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //86
    { "GPIO4_IO06", 0, { 0x230000, 0x06 }, { 0x330160, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //87
    { "GPIO4_IO07", 0, { 0x230000, 0x07 }, { 0x330164, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //88
    { "GPIO4_IO08", 0, { 0x230000, 0x08 }, { 0x330168, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //89
    { "GPIO4_IO09", 0, { 0x230000, 0x09 }, { 0x33016C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //90
    { "GPIO4_IO10", 0, { 0x230000, 0x0A }, { 0x330170, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //91
    { "GPIO4_IO11", 0, { 0x230000, 0x0B }, { 0x330174, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //92
    { "GPIO4_IO12", 0, { 0x230000, 0x0C }, { 0x330178, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //93
    { "GPIO4_IO13", 0, { 0x230000, 0x0D }, { 0x33017C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //94
    { "GPIO4_IO14", 0, { 0x230000, 0x0E }, { 0x330180, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //95
    { "GPIO4_IO15", 0, { 0x230000, 0x0F }, { 0x330184, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //96
    { "GPIO4_IO16", 0, { 0x230000, 0x10 }, { 0x330188, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //97
    { "GPIO4_IO17", 0, { 0x230000, 0x11 }, { 0x33018C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //98
    { "GPIO4_IO18", 0, { 0x230000, 0x12 }, { 0x330190, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //99
    { "GPIO4_IO19", 0, { 0x230000, 0x13 }, { 0x330194, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //100
    { "GPIO4_IO20", 0, { 0x230000, 0x14 }, { 0x330198, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //101
    { "GPIO4_IO21", 0, { 0x230000, 0x15 }, { 0x33019C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //102
    { "GPIO4_IO22", 0, { 0x230000, 0x16 }, { 0x3301A0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //103
    { "GPIO4_IO23", 0, { 0x230000, 0x17 }, { 0x3301A4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //104
    { "GPIO4_IO24", 0, { 0x230000, 0x18 }, { 0x3301A8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //105
    { "GPIO4_IO25", 0, { 0x230000, 0x19 }, { 0x3301AC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //106
    { "GPIO4_IO26", 0, { 0x230000, 0x1A }, { 0x3301B0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //107
    { "GPIO4_IO27", 0, { 0x230000, 0x1B }, { 0x3301B4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //108
    { "GPIO4_IO28", 0, { 0x230000, 0x1C }, { 0x3301B8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //109
    { "GPIO4_IO29", 0, { 0x230000, 0x1D }, { 0x3301BC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //110
    { "GPIO4_IO30", 0, { 0x230000, 0x1E }, { 0x3301C0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //111
    { "GPIO4_IO31", 0, { 0x230000, 0x1F }, { 0x3301C4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //112


    // GPIO5_IOxy  ->  xy = 00..29
	{ "GPIO5_IO00", 0, { 0x240000, 0x00 }, { 0x3301C8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //113
    { "GPIO5_IO01", 0, { 0x240000, 0x01 }, { 0x3301CC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //114
    { "GPIO5_IO02", 0, { 0x240000, 0x02 }, { 0x3301D0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //115
    { "GPIO5_IO03", 0, { 0x240000, 0x03 }, { 0x3301D4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //116
    { "GPIO5_IO04", 0, { 0x240000, 0x04 }, { 0x3301D8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //117
    { "GPIO5_IO05", 0, { 0x240000, 0x05 }, { 0x3301DC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //118
    { "GPIO5_IO06", 0, { 0x240000, 0x06 }, { 0x3301E0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //119
    { "GPIO5_IO07", 0, { 0x240000, 0x07 }, { 0x3301E4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //120
    { "GPIO5_IO08", 0, { 0x240000, 0x08 }, { 0x3301E8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //121
    { "GPIO5_IO09", 0, { 0x240000, 0x09 }, { 0x3301EC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //122
    { "GPIO5_IO10", 0, { 0x240000, 0x0A }, { 0x3301F0, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //123
    { "GPIO5_IO11", 0, { 0x240000, 0x0B }, { 0x3301F4, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //124
    { "GPIO5_IO12", 0, { 0x240000, 0x0C }, { 0x3301F8, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //125
    { "GPIO5_IO13", 0, { 0x240000, 0x0D }, { 0x3301FC, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //126
    { "GPIO5_IO14", 0, { 0x240000, 0x0E }, { 0x330200, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //127
    { "GPIO5_IO15", 0, { 0x240000, 0x0F }, { 0x330204, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //128
    { "GPIO5_IO16", 0, { 0x240000, 0x10 }, { 0x330208, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //129
    { "GPIO5_IO17", 0, { 0x240000, 0x11 }, { 0x33020C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //130
    { "GPIO5_IO18", 0, { 0x240000, 0x12 }, { 0x330210, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //131
    { "GPIO5_IO19", 0, { 0x240000, 0x13 }, { 0x330214, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //132
    { "GPIO5_IO20", 0, { 0x240000, 0x14 }, { 0x330218, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //133
    { "GPIO5_IO21", 0, { 0x240000, 0x15 }, { 0x33021C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //134
    { "GPIO5_IO22", 0, { 0x240000, 0x16 }, { 0x330220, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //135
    { "GPIO5_IO23", 0, { 0x240000, 0x17 }, { 0x330224, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //136
    { "GPIO5_IO24", 0, { 0x240000, 0x18 }, { 0x330228, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //137
    { "GPIO5_IO25", 0, { 0x240000, 0x19 }, { 0x33022C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //138
    { "GPIO5_IO26", 0, { 0x240000, 0x1A }, { 0x330230, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //139
    { "GPIO5_IO27", 0, { 0x240000, 0x1B }, { 0x330234, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //140
    { "GPIO5_IO28", 0, { 0x240000, 0x1C }, { 0x330238, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //141
    { "GPIO5_IO29", 0, { 0x240000, 0x1D }, { 0x33023C, 0x0 }, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0 }, //142
};



static int nxpIMX8MPSetup(void)
{
	if ((nxpIMX8MP->fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		wiringXLog(LOG_ERR, "wiringX failed to open /dev/mem for raw memory access");
		return -1;
	}

	if ((nxpIMX8MP->gpio[0] = (unsigned char *) mmap(0,
                nxpIMX8MP->page_size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                nxpIMX8MP->fd,
                nxpIMX8MP->base_addr[0])) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map the %s %s GPIO memory address",
                nxpIMX8MP->brand,
                nxpIMX8MP->chip);
		return -1;
	}

	return 0;
}

static char *nxpIMX8MPGetPinName(int pin)
{
	return nxpIMX8MP->layout[pin].name;
}

static void nxpIMX8MPSetMap(int *map, size_t size)
{
	nxpIMX8MP->map = map;
	nxpIMX8MP->map_size = size;
}

static void nxpIMX8MPSetIRQ(int *irq, size_t size)
{
	nxpIMX8MP->irq = irq;
	nxpIMX8MP->irq_size = size;
}

static int nxpIMX8MPDigitalWrite(int i, enum digital_value_t value)
{
	struct layout_t *pin = NULL;
	unsigned long addr = 0;
	uint32_t val = 0;

	pin = &nxpIMX8MP->layout[nxpIMX8MP->map[i]];

	if (nxpIMX8MP->map == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (nxpIMX8MP->fd <= 0 || nxpIMX8MP->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (pin->mode != PINMODE_OUTPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to output mode",
            nxpIMX8MP->brand, nxpIMX8MP->chip, i);
		return -1;
	}

	addr = (unsigned long)
            (nxpIMX8MP->gpio[pin->addr]
            + nxpIMX8MP->base_offs[pin->addr]
            + pin->data.offset);

	val = soc_readl(addr);
	if (value == HIGH) {
		soc_writel(addr, val | (1 << pin->data.bit));
	} else {
		soc_writel(addr, val & ~(1 << pin->data.bit));
	}

	return 0;
}

static int nxpIMX8MPDigitalRead(int i)
{
	void *gpio = NULL;
	struct layout_t *pin = NULL;
	unsigned long addr = 0;
	uint32_t val = 0;

	pin = &nxpIMX8MP->layout[nxpIMX8MP->map[i]];
	gpio = nxpIMX8MP->gpio[pin->addr];
	addr = (unsigned long)
        (gpio
        + nxpIMX8MP->base_offs[pin->addr]
        + pin->data.offset
        + GPIO_PSR_REG_OFFSET);

	if (nxpIMX8MP->map == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (nxpIMX8MP->fd <= 0 || nxpIMX8MP->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (pin->mode != PINMODE_INPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to input mode",
            nxpIMX8MP->brand, nxpIMX8MP->chip, i);
		return -1;
	}

	val = soc_readl(addr);

	return (int)((val & (1 << pin->data.bit)) >> pin->data.bit);
}

static int nxpIMX8MPPinMode(int i, enum pinmode_t mode)
{
	struct layout_t *pin = NULL;
	unsigned long addrSel = 0;
	unsigned long addrDat = 0;
	uint32_t val = 0;

	if (nxpIMX8MP->map == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (nxpIMX8MP->fd <= 0 || nxpIMX8MP->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}

	pin = &nxpIMX8MP->layout[nxpIMX8MP->map[i]];
	addrSel = (unsigned long)
        (nxpIMX8MP->gpio[pin->addr]
        + nxpIMX8MP->base_offs[pin->addr]
        + pin->select.offset);
	addrDat = (unsigned long)
        (nxpIMX8MP->gpio[pin->addr]
        + nxpIMX8MP->base_offs[pin->addr]
        + pin->data.offset
        + GPIO_GDIR_REG_OFFSET);

	pin->mode = mode;

	soc_writel(addrSel, FUNC_GPIO);

	val = soc_readl(addrDat);
	if (mode == PINMODE_OUTPUT) {
		soc_writel(addrDat, val | (1 << pin->data.bit));
	} else if (mode == PINMODE_INPUT) {
		soc_writel(addrDat, val & ~(1 << pin->data.bit));
	}

	return 0;
}

static int nxpIMX8MPISR(int i, enum isr_mode_t mode)
{
	struct layout_t *pin = NULL;
	char path[PATH_MAX];
    int ret;
	int gpio_bank = -1;
	int gpio_pin = -1;

	if (nxpIMX8MP->irq == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (nxpIMX8MP->fd <= 0 || nxpIMX8MP->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}

	pin = &nxpIMX8MP->layout[nxpIMX8MP->irq[i]];

	if (pin->fd > 0) {
		printf("Ronen *************  The %s %s GPIO %d interrupt already set \n", nxpIMX8MP->brand, nxpIMX8MP->chip, i);
		printf("Ronen *************  interrupt mode %d  interrupt name %s \n" , pin->mode , pin->name);
		close(pin->fd);
	} else {
		// Check and export gpio pin
		gpio_bank = (pin->name[4] - '0');
		gpio_pin  = (pin->name[8] - '0') * 10 + (pin->name[9] - '0');
		switch (gpio_bank) {
		case 1:
			chip = gpiod_chip_open(GPIO_CHIP_1);
			break;
		case 2:
			chip = gpiod_chip_open(GPIO_CHIP_2);
			break;
		case 3:
			chip = gpiod_chip_open(GPIO_CHIP_3);
			break;
		case 4:
			chip = gpiod_chip_open(GPIO_CHIP_4);
			break;
		case 5:
			chip = gpiod_chip_open(GPIO_CHIP_5);
			break;
		default:
			printf("Error calculating gpio bank!\n");
			return -1;
		}
		if (!chip) {
			printf("Open chip failed!\n");
			return -1;
		}

		line = gpiod_chip_get_line(chip, gpio_pin);
		if (!line) {
			printf("Get line failed!\n");
			gpiod_line_release(line);
			gpiod_chip_close(chip);
			return -1;
		}

		// Set gpio interrupt mode to 'mode'
		switch (mode) {
		case ISR_MODE_BOTH:
			ret = gpiod_line_request_both_edges_events(line, GPIOD_CONSUMER);
			break;
		case ISR_MODE_RISING:
			ret = gpiod_line_request_rising_edge_events(line, GPIOD_CONSUMER);
			break;
		case ISR_MODE_FALLING:
			ret = gpiod_line_request_falling_edge_events(line, GPIOD_CONSUMER);
			break;
		case ISR_MODE_NONE:
			printf("ISR_MODE_NONE interrupt mode selected!\n");
			break;
		case ISR_MODE_UNKNOWN:
			printf("ISR_MODE_UNKNOWN interrupt mode selected!\nExiting...\n");
			gpiod_line_release(line);
			gpiod_chip_close(chip);
			return -1;
			break;
		default:
			break;
		}
		pin->mode = PINMODE_INTERRUPT;
		if (ret < 0) {
			fprintf(stderr, "Failed to request interrupt edge event! %s\n", strerror(errno));
			gpiod_line_release(line);
			gpiod_chip_close(chip);
			return -1;
		}
	}

	pin->mode = PINMODE_INTERRUPT;
	printf("Ronen **** interrupt was set properly The %s %s GPIO %d \n" , nxpIMX8MP->brand, nxpIMX8MP->chip, i);

	return 0;
}

static int nxpIMX8MPWaitForInterrupt(int i, int ms)
{
	static int once_printf = 0 ;
	struct layout_t *pin = &nxpIMX8MP->layout[nxpIMX8MP->irq[i]];
	int gpio_bank = -1;
	int gpio_pin  = -1;
	struct timespec ts = { 3, 0 };
	struct gpiod_line_event event;
	int ret = -1;

	/*if (ms == -1) {
		//ts = NULL;
		ts.tv_sec = 0;
		ts.tv_nsec = 2000000000;
	} else {
		perror("Polling time not configured to infinite!\n");
		return -1;
	}*/

	if (pin->mode != PINMODE_INTERRUPT) {
		if (once_printf == 0) {
			wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to interrupt mode",
                nxpIMX8MP->brand, nxpIMX8MP->chip, i);
			once_printf = 0 ;
		}
		return -1;
	}
	/*if (pin->fd <= 0) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d has not been opened for reading",
			nxpIMX8MP->brand, nxpIMX8MP->chip, i);
		return -1;
	}*/
	once_printf = 0;

	ret = gpiod_line_event_wait(line, &ts);
	if (ret < 0) {
		// perror("Wait event notification failed!\n");
		gpiod_line_release(line);
		gpiod_chip_close(chip);
		return -1;
	} else if (ret == 0) {
		gpiod_line_release(line);
		gpiod_chip_close(chip);
		printf("Wait event notification on line timeout. Shouldn't be printed.\n");
		return 0;
	}

	ret = gpiod_line_event_read(line, &event);
	if (ret < 0) {
		printf("Read last event notification failed\n");
		return -1;

	}

	return ret;
}

static int nxpIMX8MPGC(void)
{
	struct layout_t *pin = NULL;
	char path[PATH_MAX];
	int i = 0;

	if (nxpIMX8MP->map != NULL) {
		for (i=0;i<nxpIMX8MP->map_size;i++) {
			pin = &nxpIMX8MP->layout[nxpIMX8MP->map[i]];
			if (pin->mode == PINMODE_OUTPUT) {
				pinMode(i, PINMODE_INPUT);
			} else if (pin->mode == PINMODE_INTERRUPT) {
				sprintf(path, "/sys/class/gpio/gpio%d", nxpIMX8MP->irq[i]);
				if ((soc_sysfs_check_gpio(nxpIMX8MP, path)) == 0) {
					sprintf(path, "/sys/class/gpio/unexport");
					soc_sysfs_gpio_unexport(nxpIMX8MP, path, i);
				}
			}
			if (pin->fd > 0) {
				close(pin->fd);
				pin->fd = 0;
			}
		}
	}
	if (nxpIMX8MP->gpio[0] != NULL) {
		munmap(nxpIMX8MP->gpio[0], nxpIMX8MP->page_size);
	}

	return 0;
}

static int nxpIMX8MPSelectableFd(int i)
{
	struct layout_t *pin = NULL;

	if (nxpIMX8MP->irq == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (nxpIMX8MP->fd <= 0 || nxpIMX8MP->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}

	pin = &nxpIMX8MP->layout[nxpIMX8MP->irq[i]];
    return pin->fd;
}

int nxpIMX8MPPinInterruptMask(int i, uint8_t mode)
{
	struct layout_t *pin = NULL;
	unsigned long addrDat = 0;
	uint32_t val = 0;

	if (nxpIMX8MP->map == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}
	if (nxpIMX8MP->fd <= 0 || nxpIMX8MP->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX",
            nxpIMX8MP->brand, nxpIMX8MP->chip);
		return -1;
	}

	pin = &nxpIMX8MP->layout[nxpIMX8MP->map[i]];
    addrDat = (unsigned long)
        (nxpIMX8MP->gpio[pin->addr]
        + nxpIMX8MP->base_offs[pin->addr]
        + pin->data.offset
        + GPIO_IMR_REG_OFFSET);
	pin->mode = mode;

	val = soc_readl(addrDat);
	if (mode == UNMASK) {
		soc_writel(addrDat, val | (1 << pin->data.bit));
	} else if (mode == MASK) {
		soc_writel(addrDat, val & ~(1 << pin->data.bit));
	}

	return 0;
}

void nxpIMX8MPInit(void)
{
	soc_register(&nxpIMX8MP, "NXP", "IMX8MP");

	nxpIMX8MP->layout = layout;

	nxpIMX8MP->support.isr_modes = ISR_MODE_RISING
			| ISR_MODE_FALLING
			| ISR_MODE_BOTH
			| ISR_MODE_NONE;

	nxpIMX8MP->page_size = (5*1024*1024);
	nxpIMX8MP->base_addr[0] = 0x30000000;
	nxpIMX8MP->base_offs[0] = 0x00000000;

	nxpIMX8MP->gc = &nxpIMX8MPGC;
	nxpIMX8MP->selectableFd = &nxpIMX8MPSelectableFd;

	nxpIMX8MP->pinMode = &nxpIMX8MPPinMode;
	nxpIMX8MP->setup = &nxpIMX8MPSetup;
	nxpIMX8MP->digitalRead = &nxpIMX8MPDigitalRead;
	nxpIMX8MP->digitalWrite = &nxpIMX8MPDigitalWrite;
	nxpIMX8MP->getPinName = &nxpIMX8MPGetPinName;
	nxpIMX8MP->setMap = &nxpIMX8MPSetMap;
	nxpIMX8MP->setIRQ = &nxpIMX8MPSetIRQ;
	nxpIMX8MP->isr = &nxpIMX8MPISR;
	nxpIMX8MP->waitForInterrupt = &nxpIMX8MPWaitForInterrupt;
}

int nxpIMX8MPGetPinMux(int i)
{
	return 0;
}

