#ifndef NAND_DRV_PLATFORM
#define NAND_DRV_PLATFORM

#define NAND_DRV_LINUX				1

//=====================================================
// do NOT edit the following content.
//=====================================================
#if defined(NAND_DRV_LINUX) && NAND_DRV_LINUX
#include "config_nand/drvNand_platform_Linux.h"
#else
#error "Error! no platform selected."
#endif

//=====================================================
// misc. do NOT edit the following content.
//=====================================================
#define NAND_DMA_RACING_PATCH		1
#define NAND_DMA_PATCH_WAIT_TIME	10000 // us -> 10ms
#define NAND_DMA_RACING_PATTERN		(((U32)'D'<<24)|((U32)'M'<<16)|((U32)'B'<<8)|(U32)'N')

//===========================================================
// Time Dalay, do NOT edit the following content, for NC_WaitComplete use.
//===========================================================
#define DELAY_100us_in_us			100
#define DELAY_500us_in_us			500
#define DELAY_1ms_in_us				1000
#define DELAY_10ms_in_us			10000
#define DELAY_100ms_in_us			100000
#define DELAY_500ms_in_us			500000
#define DELAY_1s_in_us				1000000

#define WAIT_ERASE_TIME				DELAY_1s_in_us
#define WAIT_WRITE_TIME				DELAY_1s_in_us
#define WAIT_READ_TIME				DELAY_100ms_in_us
#define WAIT_RESET_TIME				DELAY_10ms_in_us

#endif // NAND_DRV_PLATFORM

