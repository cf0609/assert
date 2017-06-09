
#ifndef __MPYB_H
#define __MPYB_H

/**
 * @brief CONFIG project 
 */
#define CONFIG_MPYB_HAS_SPIFLASH 	(1)
#define CONFIG_MPYB_HAS_OLED 	(1)

#if CONFIG_MPYB_HAS_SPIFLASH
/**
 * @brief SPIFLASH CONFIG
 */
#define IS_USE_NSS	(0)
#define HANDLE_SPI	SPIHandle2

#if IS_USE_NSS 
#define FLASH_CS_ENABLE()
#define FLASH_CS_DISABLE()
#else
// SPI FLASH SELECT PIN
#define MICROPY_HW_SPI_CS_PIN     (pin_B12)
#define FLASH_CS_DISABLE()	MICROPY_HW_SPI_CS_PIN.gpio->BSRRL =  MICROPY_HW_SPI_CS_PIN.pin_mask// CS HIGH
#define FLASH_CS_ENABLE()	MICROPY_HW_SPI_CS_PIN.gpio->BSRRH = MICROPY_HW_SPI_CS_PIN.pin_mask // CS LOW
#endif /* IS_USE_NSS */
#endif /* CONFIG_MPYB_HAS_SPIFLASH */

#if CONFIG_MPYB_HAS_OLED
// #define 
#endif /* CONFIG_MPYB_HAS_OLED */

#endif /* __MPYB_H */
