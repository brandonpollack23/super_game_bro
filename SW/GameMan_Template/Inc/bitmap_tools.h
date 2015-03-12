// Brandon Pollack
// Tools for manipulating and loading bitmaps in conjuction with fatfs
// assums the the SD card is mounted and ready from fatFS

#ifndef __BMP_TOOLS_H
#define __BMP_TOOLS_H

#include "stdint.h"
#include "ff_gen_drv.h"
#include "GameMan_RevA0.h"

#define BMP_SIZE_ADDR 0x02
#define BMP_PX_ARRAY_START_ADDR 0xA //pointer to pointer of start of pixel array

#define BMP_DIB_SIZE_ADDR 0x0E
#define BMP_WIDTH_ADDR 0x12 //signed integer
#define BMP_HEIGHT_ADDR 0x16 //signed integer
#define BPP_ADDR 0x1C

enum
{
	BM_LOADOK,
	BM_NOTBM,
	BM_FILEOPEN_FAIL,
	BM_FILEREAD_FAIL,
	BM_FILESEEK_FAIL
};

uint32_t loadImage(char* file, uint8_t* dest, uint32_t* bytes_loaded); //decodes simple bmp data and loads (correctly) into memory (from BGR to RGB)

#endif