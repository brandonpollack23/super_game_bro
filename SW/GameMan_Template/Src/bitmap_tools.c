//Brandon Pollack
//Tools for dealing with standard MS bitmap files and loading them into memory
//in a way that the STM32 can understand in 16 bit mode
#include "bitmap_tools.h"

void inline swap16(uint16_t* a, uint16_t* b);

uint32_t loadImage(char* filepath, uint8_t* dest, uint32_t* bytes_loaded)
{
	uint32_t readbytes;
	uint32_t width, height;
	uint16_t* dest16 = (uint16_t*) dest;
	char ft[2];
	FRESULT fr;
	
	if(f_open(&file,filepath,0) != FR_OK)
	{
		return BM_FILEOPEN_FAIL;
	}
	
	fr = f_read(&file,ft,2,&readbytes); //read file type
	if(readbytes == 0 || fr != FR_OK)
	{
		return BM_FILEREAD_FAIL;
	}
	if(ft[0] != 'B' || ft[1] != 'M')
	{
		return BM_NOTBM; //not a bmp file
	}
	
	if(f_lseek(&file,BMP_WIDTH_ADDR) != FR_OK)
	{
		return BM_FILESEEK_FAIL;
	}
	fr = f_read(&file,&width,4,&readbytes); //read in width
	if(readbytes == 0 || fr != FR_OK)
	{
		return BM_FILEREAD_FAIL;
	}
	
	f_read(&file,&height,4,&readbytes); //read in height
	if(readbytes == 0 || fr != FR_OK)
	{
		return BM_FILEREAD_FAIL;
	}
	
	*bytes_loaded = 2*width*height;
	
	if(f_lseek(&file,BMP_PX_ARRAY_START_ADDR) != FR_OK) //go to pixel data
	{
		return BM_FILESEEK_FAIL;
	}
		
	fr = f_read(&file,dest,*bytes_loaded,&readbytes); //starting at px address load 2 bytes for each pixel
	if(fr != FR_OK || readbytes == 0)
	{
		return BM_FILEREAD_FAIL;
	}	
	
	for(int i = 0; i < height/2; ++i)
	{
		uint16_t* bottom_row_ptr = dest16 + (height - i - 1)*width;
		uint16_t* top_row_ptr = dest16 + i*width;
		for(int j = 0; j < width/2; ++j)
		{
			swap16(top_row_ptr + j,bottom_row_ptr + j); //swap the row order
		}
	}	
	
	return BM_LOADOK;
}

void inline swap16(uint16_t* a, uint16_t* b)
{
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}