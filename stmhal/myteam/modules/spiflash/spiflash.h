/**
 * @file spiflash.h
 * @author longqi ( log/assert )
 * @date 2016-xx-xx
 * @brief N/A
 */
#ifndef __SPIFLASH_H
#define __SPIFLASH_H

#include "spi.h"
#include <stdint.h>


/**
 * @brief typeof flash
 */
typedef struct {
	char *name;		// spiflash name
	uint32_t id;		// flash id
	uint16_t pagesize;	// 
	uint16_t sectorsize;//
	uint32_t flashsize;
	/* command */
	uint8_t read_cmd;	// read sector command
	uint8_t write_cmd;	// write sector command
	uint8_t read_sr;	// read status register
	uint8_t write_en;	// write enable
	uint8_t erase_sec;	// erase sector
	uint8_t erase_blk;	// erase block
	uint8_t erase_chip;	// erase block
	uint8_t read_id;	// read flash id command
}spiflash_support_t;

/**
 * @brief 
 */
typedef struct {
	SPI_HandleTypeDef *phspi;
	spiflash_support_t *current_flash;
	bool init; // must be initial once
}spiflash_obj_t;
 
/**
 * @brief set flash param
 */
#define GET_SPIFLASH_SUPPORT_TABLE()	\
{\
	/* name------id------pagesize-sectorsize-flashsize-read_cmd-write_cmd-read_sr-write_en-erase_sec-erase_blk-erase_chip-read_id */\
	{ "W25Q128",0xef4018, 0x100,   0x1000,    0x1000000,0x03,     0x02,   0x05,   0x06,    0x20,      0xd8,    0xc7,     0x9f},\
	{ "W25Q64", 0xef4017, 0x100,   0x1000,    0x800000, 0x03,     0x02,   0x05,   0x06,    0x20,      0xd8,    0xc7,     0x9f},\
	{ "W25Q32", 0xef4016, 0x100,   0x1000,    0x400000, 0x03,     0x02,   0x05,   0x06,    0x20,      0xd8,    0xc7,     0x9f},\
}

/**
 * @brief set flash param
 */
extern spiflash_obj_t spiflash[1];
extern bool spiflash_init (void);
extern bool spiflash_erase_sector(uint32_t flash_addr, uint32_t num_of_bytes);
extern uint32_t spiflash_write_buffer (uint32_t flash_addr, uint8_t *pbuffer, uint32_t num_of_bytes);
extern uint32_t spiflash_read_buffer (uint32_t flash_addr, uint8_t *pbuffer, uint32_t num_of_bytes);
extern uint32_t spiflash_get_sector_info(uint32_t flash_addr, uint32_t *psector_start);
extern uint32_t spiflash_conver_addr_by_sector(uint32_t *pflash_addr, uint32_t sector_start);
extern void spiflash_chip (void);

extern bool spiflash_test (void);


#endif /* __SPIFLASH_H */
