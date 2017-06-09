/**
 * @file ssd1306.h
 * @author longqi ( log/assert )
 * @date 2016-06-18
 * @brief N/A
 */
 
#ifndef __SSD1306_H
#define __SSD1306_H

// #include "i2c.h"

extern void *gc_alloc(size_t n_bytes, bool has_finaliser);
extern void gc_free(void *ptr);
typedef struct {
	I2C_HandleTypeDef *phiic; // to iic handle
	uint8_t width;
	uint8_t hight;
	uint8_t *pdisplay_mentry;
}ssd1306_obj_t;
#define DISPLAY_MEM_SIZE (128*8)
/**
 * @brief ssd1306 
 */
#define SSD1306_ADDR 0x78
#define SSD1306_REG_MODE 0x00
#define SSD1306_DATA_MODE 0x40
/**
 * @brief ssd1306 initial array
 */
#define GET_SSD1306_INIT_ARRAY() \
	{\
		0xAE,  /* --display off */ \
		0x00,  /* ---set low column address */ \
		0x10,  /* ---set high column address */ \
		0x40,  /* --set start line address */ \
		0xB0,  /* --set page address */ \
		0x81,  /*  contract control */ \
		0xFF,  /* --128    */ \
		0xA1,  /* set segment remap  */ \
		0xA6,  /* --normal / reverse */ \
		0xA8,  /* --set multiplex ratio(1 to 64) */ \
		0x3F,  /* --1/32 duty */ \
		0xC8,  /* Com scan direction */ \
		0xD3,  /* -set display offset */ \
		0x00,  /*  */ \
		0xD5,  /* set osc division */ \
		0x80,  /*  */ \
		0xD8,  /* set area color mode off */ \
		0x05,  /*  */ \
		0xD9,  /* Set Pre-Charge Period */ \
		0xF1,  /*  */ \
		0xDA,  /* set com pin configuartion */ \
		0x12,  /*  */ \
		0xDB,  /* set Vcomh */ \
		0x30,  /*  */ \
		0x8D,  /* set charge pump enable */ \
		0x14,  /*  */ \
		0xAF  /* --turn on oled panel */ \
	}

extern const mp_obj_type_t pyb_ssd1306_type;

#endif /* __SSD1306_H */


