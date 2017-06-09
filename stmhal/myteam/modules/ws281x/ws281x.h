/**
 * @file ws2811.h
 * @author longqi ( log/assert )
 * @date 2016-06-29
 * @brief N/A
 */
#ifndef __WS281X_H
#define __WS281X_H


/**
 * @brief type
 */
typedef struct {
	uint8_t size;
	uint8_t *pentry;
	const pin_obj_t *pctrl_pin;
}ws281x_obj_t;
/**
 * @brief type
 */
typedef struct {
	const char *name;
	const pin_obj_t *pin;
}mpyb_pin_t;

extern ws281x_obj_t ws281x[1];
/**
 * @brief 
 */
#define WS281x_HIGH() 	ws281x->pctrl_pin->gpio->BSRRL =  ws281x->pctrl_pin->pin_mask
#define WS281x_LOW()	ws281x->pctrl_pin->gpio->BSRRH =  ws281x->pctrl_pin->pin_mask

extern const mp_obj_type_t pyb_ws281x_type;
extern void ws281x_test (uint32_t rgb);

#endif