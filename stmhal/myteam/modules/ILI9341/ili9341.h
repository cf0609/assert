#ifndef _ILI9431_H_
#define _ILI9431_H_

extern void *gc_alloc(size_t n_bytes, bool has_finaliser);
extern void gc_free(void *ptr);


#define Bank1_LCD_C  ((uint32_t)0x60000000)
#define Bank1_LCD_D  ((uint32_t)0x60020000)


#define ILI9341_CMD(val)          (*((__IO uint16_t *)(Bank1_LCD_C))) = ((uint16_t)val)
#define ILI9341_Parameter(val)    (*((__IO uint16_t *)(Bank1_LCD_D))) = ((uint16_t)val)
#define ILI9341_RAM               (*((__IO uint16_t *)(Bank1_LCD_D)))


#define WHITE         0xFFFF
#define BLACK         0x0000
#define GREY          0xF7DE
#define BLUE          0x001F
#define BLUE1         0x051F
#define RED           0xF800
#define MAGENTA       0xF81F
#define GREEN         0x07E0
#define CYAN          0x7FFF
#define YELLOW        0xFFE0


void ili9341_init(void);
void ili9341_test(void);
void ili9341_set_display_window(uint16_t x,uint16_t y, uint16_t width,uint16_t height);
void ili9341_set_cursor(uint16_t x,uint16_t y);
void ili9341_set_point(uint16_t x,uint16_t y,uint16_t color);
void ili9341_clear(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);




#endif //_ILI9431_H_
