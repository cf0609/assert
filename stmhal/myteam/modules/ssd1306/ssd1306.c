/**
 * @file ssd1306.c 
 * @author longqi ( log/assert )
 * @date 2016-06-18
 * @brief N/A
 */
#include <stdio.h>
#include <string.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "irq.h"
#include "pin.h"
#include "genhdr/pins.h"
#include "bufhelper.h"
#include "dma.h"
#include "mpyb.h"
#include "ssd1306.h"
#include "oledfont.h"
#include "i2c.h"

#define TIMEOUT 200

ssd1306_obj_t ssd1306[1] = {
	{
		.width = 128,
		.hight = 64,
	}
};

// #define SSD1306_USE_HEAP
#ifndef SSD1306_USE_HEAP
static uint8_t framebuffer[128*8];
static uint8_t linebuffer[128];
#endif /* SSD1306_USE_HEAP */

/**
 * @brief ssd1306 iic bus initial
 */
bool ssd1306_init0 (I2C_HandleTypeDef *i2c)
{
	/* check iic bus class */
	if ( !i2c->Instance ) {
		i2c_init0();
	}
	/* iic bus initial */	
    i2c->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    i2c->Init.ClockSpeed      = 400000;
    i2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
    i2c->Init.DutyCycle       = I2C_DUTYCYCLE_16_9;
    i2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
    i2c->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLED;
    i2c->Init.OwnAddress1     = PYB_I2C_MASTER_ADDRESS;
    i2c->Init.OwnAddress2     = 0xfe; // unused
	i2c_init(i2c);
	return true;
}
   	
/**
 * @brief ssd1306 set position
 */
bool ssd1306_set_position(uint8_t x, uint8_t y) 
{ 	
	if( x < 0 || y < 0 || x > ssd1306->width || y > ssd1306->hight ) {
		return false;
	}	
	uint8_t array[] = {
		0xb0+y,
		((x&0xf0)>>4)|0x10,
		x&0x0f
	};
    HAL_StatusTypeDef status;
	status = HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, array, 3, 200);
	if( status != HAL_OK )
		return false;
	return true;
}   	
/**
 * @brief ssd1306 set position on/off
 */
static bool ssd1306_mem_set_position (uint8_t x, uint8_t y, bool inv)
{
	uint8_t page, bit;
	if( x < 0 || y < 0 || x > ssd1306->width || y > ssd1306->hight ) {
		return false;
	}
	page = y/8;
	bit = y%8;
	if ( inv )
		ssd1306->pdisplay_mentry[page*128+x] |= 1 << bit;
	else
		ssd1306->pdisplay_mentry[page*128+x] &= ~(1 << bit);
	return true;
}		
/**
 * @brief ssd1306 set position on/off
 */
static bool ssd1306_mem_get_position (uint8_t x, uint8_t y)
{
	uint8_t page, bit;
	if( x < 0 || y < 0 || x > ssd1306->width || y > ssd1306->hight ) {
		return false;
	}
	page = y/8;
	bit = y%8;
	return ssd1306->pdisplay_mentry[page*128+x] & ( 1 << bit );
}	
/**
 * @brief ssd1306 draw coordinate
 */
bool ssd1306_mem_draw_coordinate (bool inv)
{
	uint8_t n;
	for(n=0;n<128;n++){
		ssd1306_mem_set_position(n, 32, inv);
		ssd1306_mem_set_position(64, n, inv);
	}
	return true;
}	
 
/**
 * @brief ssd1306 update memory
 */
bool ssd1306_mem_update (void)
{
	uint8_t array[1];
    HAL_StatusTypeDef status;
	for(int i=0;i<8;i++) { 
		array[0] = 0xb0+i;
		status = HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		array[0] = 0x00;
		status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		array[0] = 0x10;
		status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_DATA_MODE, I2C_MEMADD_SIZE_8BIT, &ssd1306->pdisplay_mentry[128*i], 128, 200);
	} 
	if( status != HAL_OK )
		return false;
	return true;
}
/**
 * @brief ssd1306 clear screen
 */
bool ssd1306_clear (bool inv)
{
	uint8_t n, array[1];
    HAL_StatusTypeDef status;
	for(int i=0;i<8;i++) {  
		array[0] = 0xb0+i;
		status = HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		array[0] = 0x00;
		status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		array[0] = 0x10;
		status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		array[0] = (inv ? 0xff:0x0);
		for(n=0;n<128;n++) {
			status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_DATA_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], 1, 200);
		}
	} 
	if( status != HAL_OK )
		return false;
	return true;
}

/**
 * @brief ssd1306 set string to memory
 */
#define SSD1306LOADBIT(num) bool bit##num:1
			typedef union {
				uint8_t byte;
				struct {
					SSD1306LOADBIT( 0 );
					SSD1306LOADBIT( 1 );
					SSD1306LOADBIT( 2 );
					SSD1306LOADBIT( 3 );
					SSD1306LOADBIT( 4 );
					SSD1306LOADBIT( 5 );
					SSD1306LOADBIT( 6 );
					SSD1306LOADBIT( 7 );
				}bit[1];
			}__ssd1306_parsebit;
static bool ssd1306_mem_showchar_16bit(uint8_t x, uint8_t y, bool inv, uint8_t ch)
{
	uint8_t display_limit = (x+8 > 127 ? 127:(x+8));
	__ssd1306_parsebit bytedate[1];
	uint8_t x_offset;
	uint8_t font_index = ch - ' ';
	for(x_offset = x;x_offset<display_limit;x_offset++) {
		if( inv ) {
			bytedate[0].byte = font8X16[font_index][(x_offset-x)];
		}
		else {
			bytedate[0].byte = ~font8X16[font_index][(x_offset-x)];
		}
		ssd1306_mem_set_position(x_offset, y + 0, bytedate[0].bit->bit0);
		ssd1306_mem_set_position(x_offset, y + 1, bytedate[0].bit->bit1);
		ssd1306_mem_set_position(x_offset, y + 2, bytedate[0].bit->bit2);
		ssd1306_mem_set_position(x_offset, y + 3, bytedate[0].bit->bit3);
		ssd1306_mem_set_position(x_offset, y + 4, bytedate[0].bit->bit4);
		ssd1306_mem_set_position(x_offset, y + 5, bytedate[0].bit->bit5);
		ssd1306_mem_set_position(x_offset, y + 6, bytedate[0].bit->bit6);
		ssd1306_mem_set_position(x_offset, y + 7, bytedate[0].bit->bit7);
	}
	for(x_offset = x;x_offset<display_limit;x_offset++) {
		if( inv ) {
			bytedate[0].byte = font8X16[font_index][(display_limit - x + x_offset - x)];
		}
		else {
			bytedate[0].byte = ~font8X16[font_index][(display_limit - x + x_offset - x)];
		}
		ssd1306_mem_set_position(x_offset, y + 8 , bytedate[0].bit->bit0);
		ssd1306_mem_set_position(x_offset, y + 9 , bytedate[0].bit->bit1);
		ssd1306_mem_set_position(x_offset, y + 10, bytedate[0].bit->bit2);
		ssd1306_mem_set_position(x_offset, y + 11, bytedate[0].bit->bit3);
		ssd1306_mem_set_position(x_offset, y + 12, bytedate[0].bit->bit4);
		ssd1306_mem_set_position(x_offset, y + 13, bytedate[0].bit->bit5);
		ssd1306_mem_set_position(x_offset, y + 14, bytedate[0].bit->bit6);
		ssd1306_mem_set_position(x_offset, y + 15, bytedate[0].bit->bit7);
	}
	
	
		
	return true;
}
static bool ssd1306_mem_showchar_8bit(uint8_t x, uint8_t y, bool inv, uint8_t ch)
{
	uint8_t display_limit = (x+6 > 127 ? 127:(x+6));
	__ssd1306_parsebit bytedate[1];
	uint8_t x_offset = x;
	uint8_t font_index = ch - ' ';
	for(;x_offset<display_limit;x_offset++) {
		if( inv ) {
			bytedate[0].byte = font6x8[font_index][(x_offset-x)];
		}
		else {
			bytedate[0].byte = ~font6x8[font_index][(x_offset-x)];
		}
		ssd1306_mem_set_position(x_offset, y + 0, bytedate[0].bit->bit0);
		ssd1306_mem_set_position(x_offset, y + 1, bytedate[0].bit->bit1);
		ssd1306_mem_set_position(x_offset, y + 2, bytedate[0].bit->bit2);
		ssd1306_mem_set_position(x_offset, y + 3, bytedate[0].bit->bit3);
		ssd1306_mem_set_position(x_offset, y + 4, bytedate[0].bit->bit4);
		ssd1306_mem_set_position(x_offset, y + 5, bytedate[0].bit->bit5);
		ssd1306_mem_set_position(x_offset, y + 6, bytedate[0].bit->bit6);
		ssd1306_mem_set_position(x_offset, y + 7, bytedate[0].bit->bit7);
		
	}
	return true;
}
static bool ssd1306_mem_showchar(uint8_t x, uint8_t y, bool inv, char ch, uint8_t size)
{	
	if( size == 16 ) {
		ssd1306_mem_showchar_16bit(x, y, inv, ch);
	}
	else {
		ssd1306_mem_showchar_8bit(x, y, inv, ch);
	}
	return true;
}
static bool ssd1306_mem_showstring(uint8_t x, uint8_t y, bool inv, const char *pstring, uint8_t size)
{	
	while(*pstring) {
		ssd1306_mem_showchar(x, y, inv, *pstring, size);
		if( size == 16 ) {
			x += 8;
		}
		else {
			x += 6;
		}
		pstring++;
	}
	return true;
}
/**
 * @brief ssd1306 clear memory
 */
/**
 * @brief ssd1306 clear memory
 */
bool  ssd1306_mem_clear(bool inv)
{
	/* why : use dma */
	memset(ssd1306->pdisplay_mentry, inv ? 0xff:0, DISPLAY_MEM_SIZE);
	return true;
}

/**
 * @brief ssd1306 iic bus initial
 */
bool ssd1306_init (bool use_mem)
{
	if( !ssd1306->phiic ) {
		goto ssd1306_init_faild;
	}
	/* initial bus first */ 
	ssd1306_init0(ssd1306->phiic);
	/* delay is necessary */
	HAL_Delay(100);
	/* initial ssd1306 without dma */ 
    HAL_StatusTypeDef status;
	/* 100 times for try */
    for (int i = 0; i < 10; i++) {
        status = HAL_I2C_IsDeviceReady(ssd1306->phiic, SSD1306_ADDR, 10, TIMEOUT);
        if ( status == HAL_OK ) {
            break;
        }
    }
	if (status != HAL_OK) {
		goto ssd1306_init_faild;
	}
	uint8_t array[] = GET_SSD1306_INIT_ARRAY();
		status |= HAL_I2C_Mem_Write(ssd1306->phiic, SSD1306_ADDR, SSD1306_REG_MODE, I2C_MEMADD_SIZE_8BIT, &array[0], sizeof(array), 200);
	if( status != HAL_OK )
		goto ssd1306_init_failt;
	if ( !ssd1306->pdisplay_mentry && use_mem ) { // once
#ifdef SSD1306_USE_HEAP
		ssd1306->pdisplay_mentry = (uint8_t *)malloc(DISPLAY_MEM_SIZE,0);
#else
		ssd1306->pdisplay_mentry = (uint8_t *)framebuffer;
#endif /* SSD1306_USE_HEAP */
		if ( !ssd1306->pdisplay_mentry ) {
			goto ssd1306_init_failt;
		}
		
	}
	if( !use_mem ) {
		ssd1306_clear(0);
	}
	else {
		ssd1306_mem_clear(0);
		ssd1306_mem_update();
	}
	return true;
	/* output an error msg */
ssd1306_init_faild:
	nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "ssd1306 not found"));
ssd1306_init_failt:
	nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "ssd1306 initial failt"));
	return false;
}

/**
 * @brief ssd1306 load to memory
 */
#include "../lib/fatfs/ff.h"
#include "../lib/fatfs/integer.h"
/* BitMap Table 1 */
typedef struct tagBITMAPFILEHEADER { 
	WORD	bfReserved0;
	WORD    bfType; 
	DWORD   bfSize; 
	WORD    bfReserved1; 
	WORD    bfReserved2; 
	DWORD   bfOffBits; 
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;
/* BitMap Table 2 */
typedef struct tagBITMAPINFOHEADER { 
DWORD      biSize; 
LONG        biWidth; 
LONG        biHeight; 
WORD       biPlanes; 
WORD       biBitCount; 
DWORD      biCompression; 
DWORD      biSizeImage; 
LONG        biXPelsPerMeter; 
LONG        biYPelsPerMeter; 
DWORD      biClrUsed; 
DWORD      biClrImportant; 
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
/* all in one */
typedef struct tagBITMAPFILEINFO {
	BITMAPFILEHEADER filehead;
	BITMAPINFOHEADER infohead;
}BFI, *PBFI;

static bool ssd1306_load2fb (const TCHAR *filename, PBFI info, bool is_load)
{
	FIL fd;
	FRESULT res = f_open(&fd, filename, FA_READ);
    if (res != FR_OK) { /* readonly */
        return false;
    }
	/* for fatfs */
	UINT n; 
	f_read(&fd, &info->filehead.bfType, sizeof(BFI) - sizeof(WORD), &n);
#if 0
	printf("Type %c%c\n", info->filehead.bfType&0xFF, info->filehead.bfType>>8);
	printf("Data 0x%lx\n", info->filehead.bfOffBits);
	printf("BitCount %d\n", info->infohead.biBitCount);
	printf("Compression %ld\n", info->infohead.biCompression);
	printf("width = %ld\thight = %ld\n", info->infohead.biWidth, info->infohead.biHeight);
#endif
	if( !is_load ) { 
		f_close(&fd);
		return true;
	}
	/* date map */
	switch( info->infohead.biBitCount ) {
		case 1: { /* */
			f_lseek(&fd, info->filehead.bfOffBits);
#ifdef SSD1306_USE_HEAP
			uint8_t *page = (uint8_t *)malloc(ssd1306->width,1);
#else
			uint8_t *page = (uint8_t *)linebuffer;
#endif /* SSD1306_USE_HEAP */
			if( !page ) {
				goto ssd1306_load2fb_faild;
			}
			__ssd1306_parsebit bytedate[1];
			for(int i=7;i>=0;i--) {
				f_read(&fd, page, ssd1306->width, &n); // 
				for(int j=0;j<128;j++) {
					bytedate->byte = page[j];
					ssd1306_mem_set_position (7 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit0);
					ssd1306_mem_set_position (6 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit1);
					ssd1306_mem_set_position (5 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit2);
					ssd1306_mem_set_position (4 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit3);
					ssd1306_mem_set_position (3 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit4);
					ssd1306_mem_set_position (2 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit5);
					ssd1306_mem_set_position (1 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit6);
					ssd1306_mem_set_position (0 + ((j % 16 ) << 3), (127 - j) / 16 + (i << 3), bytedate->bit->bit7);
				}
			}
			free(page);
		}
		break;
		default: break;
	}
	f_close(&fd);
	return true;
ssd1306_load2fb_faild:
	f_close(&fd);
	return false;
}




/******************************************************************************/
/* Micro Python bindings                                                      */

typedef struct _pyb_ssd_obj_t {
    mp_obj_base_t base;
} pyb_ssd1306_obj_t;

STATIC pyb_ssd1306_obj_t pyb_ssd1306_obj;

STATIC mp_obj_t pyb_ssd1306_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
	mp_obj_t bus_num = args[0];
	mp_obj_t is_use_mem_display = args[1];
	if( !MP_OBJ_IS_INT(bus_num) || !MP_OBJ_IS_INT(is_use_mem_display) ) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LCD bus type error"));
	}
	else if( mp_obj_get_int(bus_num) == 1 ){
		ssd1306->phiic = &I2CHandle1;
	}
	else if( mp_obj_get_int(bus_num) == 2 ){
		ssd1306->phiic = &I2CHandle2;
	}
	else if( mp_obj_get_int(bus_num) == 3 ){
		ssd1306->phiic = &I2CHandle3;
	}
	else{
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LCD bus does not exist"));
	}
		
    // init ssd1306 object
    pyb_ssd1306_obj.base.type = &pyb_ssd1306_type;
    ssd1306_init(mp_obj_get_int(is_use_mem_display) == 0 ? false:true);

    return &pyb_ssd1306_obj;
}

STATIC mp_obj_t pyb_ssd1306_mem_update(mp_obj_t self_in) 
{
	ssd1306_mem_update();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_ssd1306_mem_update_obj, pyb_ssd1306_mem_update);

/**
 * /// Usage: setbit(index,(r,g,b)) 
 */
STATIC mp_obj_t pyb_ssd1306_set_text(mp_uint_t n_args, const mp_obj_t *args) 
{
	const char *showstr = mp_obj_str_get_str(args[2]);
	mp_obj_t *items;
	mp_obj_get_array_fixed_n(args[1], 2, &items);
	uint8_t x, y;
	x = mp_obj_get_int(items[0]);
	y = mp_obj_get_int(items[1]);
	bool inv = 1;
	uint8_t size = 8;
    if ( n_args == 4) {
		inv = (mp_obj_get_int(args[3]) == 0 ? false : true);
    }
	else if ( n_args == 5) {
		size = mp_obj_get_int(args[4]);
	}
	ssd1306_mem_showstring(x, y, inv, showstr, size);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_ssd1306_set_text_obj, 3, 5, pyb_ssd1306_set_text);

STATIC mp_obj_t pyb_ssd1306_mem_draw_coordinate(mp_obj_t self_in, mp_obj_t mp_inv) 
{
	if( !MP_OBJ_IS_INT(mp_inv) ) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "type error"));
		return mp_const_none;
	}
	switch( mp_obj_get_int(mp_inv) ){
		case 0 ... 1:
			ssd1306_mem_draw_coordinate(mp_obj_get_int(mp_inv));
			break;
		default:
			nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "number error"));
			break;
	}
	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_ssd1306_mem_draw_coordinate_obj, pyb_ssd1306_mem_draw_coordinate);

STATIC mp_obj_t pyb_ssd1306_mem_clear(mp_obj_t self_in, mp_obj_t mp_inv) 
{
	if( !MP_OBJ_IS_INT(mp_inv) ) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "type error"));
		return mp_const_none;
	}
	ssd1306_mem_clear(mp_obj_get_int(mp_inv) ? 1 : 0);
	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_ssd1306_mem_clear_obj, pyb_ssd1306_mem_clear);

STATIC mp_obj_t pyb_ssd1306_clear(mp_obj_t self_in, mp_obj_t mp_inv) 
{
	if( !MP_OBJ_IS_INT(mp_inv) ) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "type error"));
		return mp_const_none;
	}
	ssd1306_clear(mp_obj_get_int(mp_inv) ? 1 : 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_ssd1306_clear_obj, pyb_ssd1306_clear);
/**
 * /// Usage: logo() 
 */
STATIC mp_obj_t pyb_ssd1306_mem_logo(mp_obj_t self_in) 
{
	BFI info; /* head and info */ 
	ssd1306_load2fb ("logo.bmp", &info, true);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_ssd1306_mem_logo_obj, pyb_ssd1306_mem_logo);
/**
 * /// Usage: load('xxx.bmp','info|load') 
 */
STATIC mp_obj_t pyb_ssd1306_mem_bmp_load (mp_uint_t n_args, const mp_obj_t *args)
{
	BFI info; /* head and info */ 
	bool is_load = false; /* load bmp data to memory */ 
	if( n_args == 3 ) {
		is_load = mp_obj_get_int(args[2]);
	}
	if ( !ssd1306_load2fb (mp_obj_str_get_str(args[1]), &info, is_load) ) { /* call load function */ 
		return mp_const_none;
	}
	/* map head and info */ 
	mp_obj_t tuple[8] = {
		mp_obj_new_int(info.filehead.bfType),
		mp_obj_new_int(info.filehead.bfSize),
		mp_obj_new_int(info.filehead.bfOffBits),
		mp_obj_new_int(info.infohead.biSize),
		mp_obj_new_int(info.infohead.biWidth),
		mp_obj_new_int(info.infohead.biHeight),
		mp_obj_new_int(info.infohead.biBitCount),
		mp_obj_new_int(info.infohead.biCompression),
	};
	return mp_obj_new_tuple(8, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_ssd1306_mem_bmp_load_obj, 2, 3, pyb_ssd1306_mem_bmp_load);

STATIC mp_obj_t pyb_ssd1306_mem_set_position(mp_uint_t n_args, const mp_obj_t *args) 
{
	mp_obj_t *items;
	mp_obj_get_array_fixed_n(args[1], 2, &items);
	uint8_t x, y;
	x = mp_obj_get_int(items[0]);
	y = mp_obj_get_int(items[1]);
    if ( n_args == 2) {
		return mp_obj_new_int( ssd1306_mem_get_position(x, y) );
    } else {
        // set date and time
		
		bool inv; // ssd1306_set_mem_position
		inv = mp_obj_get_int(args[2]) ? 1:0;
		
		ssd1306_mem_set_position(x, y, inv);
    }
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_ssd1306_mem_set_position_obj, 2, 3, pyb_ssd1306_mem_set_position);

STATIC const mp_map_elem_t pyb_ssd1306_locals_dict_table[] = {
    // TODO add init, deinit, and perhaps reset methods
    { MP_OBJ_NEW_QSTR(MP_QSTR_update), (mp_obj_t)&pyb_ssd1306_mem_update_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_memclear), (mp_obj_t)&pyb_ssd1306_mem_clear_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_clear), (mp_obj_t)&pyb_ssd1306_clear_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_show), (mp_obj_t)&pyb_ssd1306_set_text_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_coordinate), (mp_obj_t)&pyb_ssd1306_mem_draw_coordinate_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mempos), (mp_obj_t)&pyb_ssd1306_mem_set_position_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_load), (mp_obj_t)&pyb_ssd1306_mem_bmp_load_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_logo), (mp_obj_t)&pyb_ssd1306_mem_logo_obj },
};

STATIC MP_DEFINE_CONST_DICT(pyb_ssd1306_locals_dict, pyb_ssd1306_locals_dict_table);

const mp_obj_type_t pyb_ssd1306_type = {
    { &mp_type_type },
    .name = MP_QSTR_ssd1306,
    .make_new = pyb_ssd1306_make_new,
    .locals_dict = (mp_obj_t)&pyb_ssd1306_locals_dict,
};
