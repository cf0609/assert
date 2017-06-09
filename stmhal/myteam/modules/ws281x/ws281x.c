/**
 * @file ws281x.h
 * @author longqi ( log/assert )
 * @date 2016-06-29
 * @brief N/A
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/gc.h"
#include "irq.h"
#include "pin.h"
#include "genhdr/pins.h"
#include "bufhelper.h"
#include "ws281x.h"

ws281x_obj_t ws281x[1] = {
	{
		0,
		(uint8_t *)0,
		&pin_B11, // default pb11
	}
};


void ws281x_init0(void) {
    /* GPIO structure */
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Configure I/O speed, mode, output type and pull */
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;

    /* Set output and initialize */
	mp_hal_gpio_clock_enable(ws281x->pctrl_pin->gpio);
	WS281x_LOW();
	GPIO_InitStructure.Pin = ws281x->pctrl_pin->pin_mask;
	HAL_GPIO_Init(ws281x->pctrl_pin->gpio, &GPIO_InitStructure);
}

/**
 * @brief ws281x io initial
 */
#define WS281X_MAX 128
static uint8_t display_array[3*WS281X_MAX];
static bool ws281x_init (uint8_t size, const pin_obj_t *pin)
{
	if( size > WS281X_MAX ) {
		goto ws281x_init_faild;
	}
	/* malloc display memory */ 
	if( !ws281x->pentry ) {
		// ws281x->pentry = (uint8_t *)malloc(3 * size, 1);
		ws281x->pentry = display_array; // (uint8_t *)malloc(3 * size, 1);
		if( !ws281x->pentry ) {
		}
		ws281x->size = size;
	}
	else { // realloc
		if( ws281x->size != size ) {
			// free(ws281x->pentry);
			// ws281x->pentry = (uint8_t *)malloc(3 * size, 1);
			ws281x->pentry = display_array; // (uint8_t *)malloc(3 * size, 1);
			if( !ws281x->pentry ) {
				goto ws281x_init_faild;
			}
			ws281x->size = size;
		}
	}
	/* set control pin */
	if( pin ) {
		ws281x->pctrl_pin = pin;
	}
	/* init io */
	ws281x_init0();
	return true;
	/* output an error msg */
ws281x_init_faild:
	nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "ws281x1 memory error"));
	return false;
}

/**
 * @brief ws281x delay 500ns ( 1/50 us \ 1/168 us )
 *		5.952380952381 ns ==>>> 6 ns
 */
static void ws281x_delay500ns(uint32_t n)
{
	while( n-- ) { /* more than 472 ns */
	/*
		// asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		int i = 0;
		for(i=0;i<8;i++) { // not 83
			asm("nop"); 
		}
	*/
		
		int i = 0;
		for(i=0;i<10;i++) { // not 83
			asm("nop"); 
		}
	}
}

/**
 * @brief ws281x send 0
 */
static void ws281x_send_zero (void)
{
	WS281x_HIGH();
	ws281x_delay500ns(1); // 0.5 us
	WS281x_LOW();
	ws281x_delay500ns(4); // 2.0 us
	// WS281x_HIGH();
}

/**
 * @brief ws281x send 1
 */
static void ws281x_send_one (void)
{
	WS281x_HIGH();
	ws281x_delay500ns(4); // 2.0 us
	WS281x_LOW();
	ws281x_delay500ns(1); // 0.5 us
	// WS281x_HIGH();
}

/**
 * @brief ws281x send reset
 */
static void ws281x_send_reset (void)
{
	WS281x_LOW();
	ws281x_delay500ns(120); // 60 us
	WS281x_HIGH();
	WS281x_LOW();
}

/**
 * @brief ws281x set a led
 */
#define ws281x_LOADBIT(num) bool bit##num:1
typedef union {
	uint8_t byte;
	struct {
		ws281x_LOADBIT( 0 );
		ws281x_LOADBIT( 1 );
		ws281x_LOADBIT( 2 );
		ws281x_LOADBIT( 3 );
		ws281x_LOADBIT( 4 );
		ws281x_LOADBIT( 5 );
		ws281x_LOADBIT( 6 );
		ws281x_LOADBIT( 7 );
	}bit[1];
}__ws281x_parsebit;
static void ws281x_set_led (uint8_t *pdata)
{
	uint8_t rgb_index = 0;
	__ws281x_parsebit bytedate[1];
	/* */
	for( ;rgb_index<3;rgb_index++) {
		bytedate->byte = pdata[rgb_index];
		if( bytedate->bit->bit7 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit6 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit5 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit4 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit3 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit2 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit1 ) {ws281x_send_one();} else {ws281x_send_zero();}
		if( bytedate->bit->bit0 ) {ws281x_send_one();} else {ws281x_send_zero();}
	}
}

/**
 * @brief ws281x set memory
 */
typedef union {
	uint32_t rgb;
	struct {
		uint8_t b;
		uint8_t r;
		uint8_t g;
		uint8_t nouse;
	}color[1];
}__ws281x_parsergb;
static void ws281x_set_memory (uint8_t index, uint32_t rgb)
{
	if( index > ws281x->size ) { /*  */
		return;
	}
	index--;
	__ws281x_parsergb _rgb[1];
	_rgb->rgb = rgb;
	ws281x->pentry[index*3] = _rgb->color->g; /* green */
	ws281x->pentry[index*3 + 1] = _rgb->color->r; /* red */	
	ws281x->pentry[index*3 + 2] = _rgb->color->b; /* blue */
}
static void ws281x_memory_clear (void)
{
	if( !ws281x->size ) { /*  */
		return;
	}
	
	memset(ws281x->pentry, 0, ws281x->size*3);
}

/**
 * @brief ws281x update
 */
static void ws281x_update (void)
{
	uint8_t index = 0;
	/* make sure is reset */
	ws281x_send_reset();
	ws281x_delay500ns(80); // 25 us
	for(;index<ws281x->size;index++) {
		ws281x_set_led(&ws281x->pentry[index*3]); // send data
	}
	/* make sure is reset */
	ws281x_send_reset();
}




/******************************************************************************/
/* Micro Python bindings   
                                                   */
const mpyb_pin_t mpyb_stm32f407_pin[] = {
	{ "B10",			&pin_B10 },
	{ "B11",			&pin_B11 },
	{ "E1",				&pin_E1 },
	{ "E2",				&pin_E2 },
	{ "E3",				&pin_E3 },
	{ "E4",				&pin_E4 },
	{ "E5",				&pin_E5 },
	{ "E6",				&pin_E6 },
	{ "D9",				&pin_D9 },
	{ "D8",				&pin_D8 },
};
uint8_t mpyb_stm32f407_pin_size = sizeof(mpyb_stm32f407_pin)/sizeof(mpyb_pin_t);

typedef struct _pyb_ws281x_obj_t {
    mp_obj_base_t base;
} pyb_ws281x_obj_t;

STATIC pyb_ws281x_obj_t pyb_ws281x_obj;

STATIC mp_obj_t pyb_ws281x_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
	mp_obj_t led_size = args[0];
	if( !MP_OBJ_IS_INT(led_size) ) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "led size error"));
	}
	pin_obj_t *pin = 0;
	if( n_args == 2 ) {
		const char *pinname = mp_obj_str_get_str(args[1]);
		for(uint8_t i=0;i<mpyb_stm32f407_pin_size;i++) {
			if( !strcmp(pinname, mpyb_stm32f407_pin[i].name) ) {
				pin = (pin_obj_t *)mpyb_stm32f407_pin[i].pin;
				break;
			}
		}
	}
    // init ssd1306 object
    pyb_ws281x_obj.base.type = &pyb_ws281x_type;
	/* initial memory and pin */
	ws281x_init(mp_obj_get_int(led_size), pin);

    return &pyb_ws281x_obj;
}

/**
 * /// Usage: setbit(index,(r,g,b)) 
 */
STATIC mp_obj_t pyb_ws281x_set_aled(mp_uint_t n_args, const mp_obj_t *args) 
{
	uint8_t index = mp_obj_get_int(args[1]); //get index
	__ws281x_parsergb _rgb[1];
    if ( n_args == 2) {
		index--;
		_rgb->color->g = ws281x->pentry[index*3]; /* green */
		_rgb->color->g = ws281x->pentry[index*3 + 1]; /* red */
		_rgb->color->b = ws281x->pentry[index*3 + 2]; /* blue */
		
		return mp_obj_new_int( _rgb->rgb );
    }
	mp_obj_t *items;
	mp_obj_get_array_fixed_n(args[2], 3, &items);
	_rgb->color->r = mp_obj_get_int(items[0]);
	_rgb->color->g = mp_obj_get_int(items[1]);
	_rgb->color->b = mp_obj_get_int(items[2]);
	ws281x_set_memory(index, _rgb->rgb);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_ws281x_set_aled_obj, 2, 3, pyb_ws281x_set_aled);
/**
 * /// Usage: setall(index,(r,g,b)) 
 */
STATIC mp_obj_t pyb_ws281x_set_allled(mp_uint_t n_args, const mp_obj_t *args) 
{
	if(n_args == 1) 
		return mp_const_none;
	uint8_t index = 1; //get index
	__ws281x_parsergb _rgb[1];
	mp_obj_t *items;
	mp_obj_get_array_fixed_n(args[1], 3, &items);
	_rgb->color->r = mp_obj_get_int(items[0]);
	_rgb->color->g = mp_obj_get_int(items[1]);
	_rgb->color->b = mp_obj_get_int(items[2]);
	for(;index<=ws281x->size;index++) {
		ws281x_set_memory(index, _rgb->rgb);
	}
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_ws281x_set_allled_obj, 1, 2, pyb_ws281x_set_allled);
/**
 * /// Usage: update() 
 */
STATIC mp_obj_t pyb_ws281x_mem_update(mp_obj_t self_in) 
{
	ws281x_update();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_ws281x_mem_update_obj, pyb_ws281x_mem_update);
/**
 * /// Usage: memclear() 
 */
STATIC mp_obj_t pyb_ws281x_mem_clear(mp_obj_t self_in) 
{
	ws281x_memory_clear();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_ws281x_mem_clear_obj, pyb_ws281x_mem_clear);

STATIC const mp_map_elem_t pyb_ws281x_locals_dict_table[] = {
    // TODO add init, deinit, and perhaps reset methods},
    { MP_OBJ_NEW_QSTR(MP_QSTR_setbit), (mp_obj_t)&pyb_ws281x_set_aled_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_setall), (mp_obj_t)&pyb_ws281x_set_allled_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_update), (mp_obj_t)&pyb_ws281x_mem_update_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_memclear), (mp_obj_t)&pyb_ws281x_mem_clear_obj },
};

STATIC MP_DEFINE_CONST_DICT(pyb_ws281x_locals_dict, pyb_ws281x_locals_dict_table);
const mp_obj_type_t pyb_ws281x_type = {
    { &mp_type_type },
    .name = MP_QSTR_ws281x,
    .make_new = pyb_ws281x_make_new,
    .locals_dict = (mp_obj_t)&pyb_ws281x_locals_dict,
};


void setrgb (uint32_t *pcolor, uint8_t r, uint8_t g, uint8_t b)
{
	*pcolor = r << 16; // g
	*pcolor |= g << 8; // r
	*pcolor |= b << 0; // b
}
#include "led.h"
void ws281x_test (uint32_t rgb)
{
	uint8_t index = 0;
	uint32_t color = rgb;
	ws281x_init(20, (pin_obj_t *)0);
	index = 20;
	ws281x_send_reset();
	while(index--) {
		ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero();
		ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero();
		ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero(); ws281x_send_zero();
	}
	// no -> green
	uint8_t r = 0,g = 0,b = 0;
	while(g < 0x8f) {
			for(index = 0;index<20;index++) {
				setrgb(&color,r,g,b);
				g++;
				ws281x_set_memory(index, color+index);
			}
			ws281x_update();
			HAL_Delay(140);
	}
	// green -> blue
	while(b < 0x8f) {
			for(index = 0;index<20;index++) {
				setrgb(&color,r,g,b);
				b++;
				g--;
				ws281x_set_memory(index, color+index);
			}
			ws281x_update();
			HAL_Delay(140);
	}
	// b -> r
	while(r < 0x8f) {
			for(index = 0;index<20;index++) {
				setrgb(&color,r,g,b);
				r++;
				b--;
				ws281x_set_memory(index, color+index);
			}
			ws281x_update();
			HAL_Delay(140);
	}
	// r -> no
	while(r > 0) {
			for(index = 0;index<20;index++) {
				setrgb(&color,r,g,b);
				r--;
				ws281x_set_memory(index, color+index);
			}
			ws281x_update();
			HAL_Delay(140);
	}
	
	return;
	
}
