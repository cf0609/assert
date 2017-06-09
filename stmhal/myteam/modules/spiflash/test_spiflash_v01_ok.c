
#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "irq.h"
#include "pin.h"
#include "genhdr/pins.h"
#include "bufhelper.h"
#include "dma.h"
#include "mpyb.h"
#include <stdio.h>

#include "spiflash.h"


#define TIMEOUT 0xFFFFFFFF
/**
 * @brief set flash table
 */
static const spiflash_support_t spiflash_support_table[] = GET_SPIFLASH_SUPPORT_TABLE();

/**
 * @brief flash object
 */
spiflash_obj_t spiflash[1] = {{.phspi = &HANDLE_SPI, .current_flash = NULL, .init = false}};

/**
 * @brief spi flash check id (SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
 */
static bool spiflash_checkid (void)
{
	uint32_t *pflash_id = NULL;
	uint8_t flash_support_count = sizeof(spiflash_support_table)/sizeof(spiflash_support_t);
	uint8_t flash_index = 0;
	uint8_t tx_buf[4] = {0, 0x5A, 0x5A, 0x5A};
	uint8_t rx_buf[8] = {0};

	for( ;flash_index<flash_support_count;flash_index++)	{
		FLASH_CS_ENABLE();
		tx_buf[0] = spiflash_support_table[flash_index].read_id;	// set id
		// memset(rx_buf, 0, sizeof(rx_buf));	// flush rx buffer
		HAL_SPI_TransmitReceive(spiflash->phspi, tx_buf, rx_buf, 4, TIMEOUT);
		rx_buf[4] = rx_buf[2];	// 0 1 2 3 4 5 6 7
		rx_buf[5] = rx_buf[1];	// 0 1 2 3 2 1 command-fb
		pflash_id = (uint32_t *)&rx_buf[3];
		if( *pflash_id == spiflash_support_table[flash_index].id )	{
			printf("flash %s have been identified \n", spiflash_support_table[flash_index].name);
			spiflash->current_flash = (spiflash_support_t *)&spiflash_support_table[flash_index];
			FLASH_CS_DISABLE(); 
			break;
		}
		FLASH_CS_DISABLE(); 
	}
	
	return (spiflash->current_flash == NULL ? false : true);
}
/**
 * @brief spi flash init
 */
bool spiflash_init (void)
{
	if( 0 && spiflash->current_flash ){
		/* check flash id */
		printf("flash %s have already identified \n", spiflash->current_flash->name);
		return true;
	}
	/* init spi Instance */
	spi_init0();
	/* deinit */
	spi_deinit(spiflash->phspi);
#if !IS_USE_NSS
	/* initialize */
	mp_hal_gpio_clock_enable(MICROPY_HW_SPI_CS_PIN.gpio);
	/* GPIO structure */
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Configure I/O speed, mode, output type and pull */
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Pin = MICROPY_HW_SPI_CS_PIN.pin_mask;

	HAL_GPIO_Init(MICROPY_HW_SPI_CS_PIN.gpio, &GPIO_InitStructure);
	FLASH_CS_DISABLE();
#endif /* !IS_USE_NSS */
	/* config SPI */
	spiflash->phspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	spiflash->phspi->Init.CLKPhase = SPI_PHASE_1EDGE;
	spiflash->phspi->Init.CLKPolarity = SPI_POLARITY_LOW;
	spiflash->phspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	spiflash->phspi->Init.CRCPolynomial = 7;
	spiflash->phspi->Init.DataSize = SPI_DATASIZE_8BIT;
	spiflash->phspi->Init.Direction = SPI_DIRECTION_2LINES;
	spiflash->phspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
	spiflash->phspi->Init.Mode = SPI_MODE_MASTER;
#if IS_USE_NSS
	spiflash->phspi->Init.NSS = SPI_NSS_HARD_OUTPUT;
#else
	spiflash->phspi->Init.NSS = SPI_NSS_SOFT;
#endif
	spiflash->phspi->Init.TIMode = SPI_TIMODE_DISABLED;
	spi_init(spiflash->phspi, IS_USE_NSS);
	
	/* check flash id */
	return spiflash_checkid();
}
const uint8_t strings[] = "hello microPython\nTest By Longqi\n";
bool spiflash_test (void)
{
	spiflash_erase_sector(0x1234, sizeof(strings));
	spiflash_write_buffer(0x1234, (uint8_t *)strings, sizeof(strings));
	uint8_t read_strings[sizeof(strings)] = {0x5A};
	spiflash_read_buffer(0x1234, read_strings, sizeof(strings));
	printf("read string is ->\n\t %s\n", read_strings);
	return true;
}
/**
 * @brief spi flash write enable HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
 */
static bool spiflash_write_enable(void)
{
	bool ret_status = false;
	uint8_t tmp;
	if( spiflash->current_flash == NULL )	return ret_status;
	
	FLASH_CS_ENABLE();
	ret_status = HAL_SPI_TransmitReceive(spiflash->phspi, &spiflash->current_flash->write_en, &tmp, 1, TIMEOUT) == HAL_OK?true:false;
	FLASH_CS_DISABLE();
	
	return ret_status;
}

/**
 * @brief spi flash operation finished
 */
static bool spiflash_wait_op_finish(void)
{
	uint8_t flash_status = 0, dummy_cmd = 0x5A;
	uint32_t timeout = 0;
	FLASH_CS_ENABLE();
	timeout = HAL_SPI_TransmitReceive(spiflash->phspi, &spiflash->current_flash->read_sr, &flash_status, 1, TIMEOUT) == HAL_OK?TIMEOUT:0;
	do{
		HAL_SPI_TransmitReceive(spiflash->phspi, &dummy_cmd, &flash_status, 1, TIMEOUT);
	}while( (flash_status & 0x1) == 0x1 && timeout <= TIMEOUT );  /*!< Write In Progress (WIP) flag */
	FLASH_CS_DISABLE();
	
	return (timeout < TIMEOUT ? true:false);
}

/**
 * @brief spi flash erase a sector
 */
static bool spiflash_erase_a_sector(uint32_t flash_addr)
{
	printf("%s -->0x%lx\n", __func__, flash_addr);
	uint8_t array[4], *parray = (uint8_t *)&flash_addr;
	uint8_t tmp;
    spiflash_write_enable();
	array[0] = parray[3]; array[1] = parray[2]; array[2] = parray[1]; array[3] = parray[0];
    
	printf("%s -->0x%x\n", __func__, array[1]);
	printf("%s -->0x%x\n", __func__, array[2]);
	printf("%s -->0x%x\n", __func__, array[3]);
	FLASH_CS_ENABLE();
    HAL_SPI_TransmitReceive(spiflash->phspi, &spiflash->current_flash->erase_sec, &tmp, 1, TIMEOUT);
    HAL_SPI_TransmitReceive(spiflash->phspi, &array[1], &array[1], 3, TIMEOUT);
	FLASH_CS_DISABLE();

    /* finished */
    spiflash_wait_op_finish();
	return true;
}
/**
 * @brief spi flash get sector offset
 */
static bool spiflash_get_sector_start2end (uint32_t flash_addr, uint32_t num_of_bytes, uint32_t *pstart_addr, uint32_t *pend_addr)
{
	if( !pstart_addr && !pend_addr ) return false;
	/* get first sector size */
	*pstart_addr = flash_addr % spiflash->current_flash->sectorsize;
	*pstart_addr = spiflash->current_flash->sectorsize - ( *pstart_addr);
	/* get flash_addr start position */
	*pstart_addr += flash_addr;
	/* get flash_addr end position */
	*pend_addr = flash_addr + num_of_bytes; // not align to end sector, often write until ( flash_addr >= end_addr )
	return true;
}
/**
 * @brief spi flash erase sector
 */
bool spiflash_erase_sector(uint32_t flash_addr, uint32_t num_of_bytes)
{
	if( !num_of_bytes ) return false;
	uint32_t start_addr, end_addr;
	spiflash_get_sector_start2end(flash_addr, num_of_bytes, &start_addr, &end_addr);
	spiflash_erase_a_sector(flash_addr);
	while( start_addr < end_addr ) {
		spiflash_erase_a_sector(start_addr);
		start_addr += spiflash->current_flash->sectorsize;
	}
	return true;
}
/**
 * @brief spi flash write a page
 */
static uint32_t spiflash_write_a_page (uint32_t flash_addr, uint8_t *pbuffer, uint32_t num_of_bytes)
{
	uint8_t array[4], *parray = (uint8_t *)&flash_addr;
	uint8_t tmp;
	/* limit page size */
	// num_of_bytes = num_of_bytes > spiflash->current_flash->pagesize ? spiflash->current_flash->pagesize : num_of_bytes;
	/* write enable */
    spiflash_write_enable();
	array[0] = parray[3]; array[1] = parray[2]; array[2] = parray[1]; array[3] = parray[0];
	printf("%s -->0x%x\n", __func__, array[1]);
	printf("%s -->0x%x\n", __func__, array[2]);
	printf("%s -->0x%x\n", __func__, array[3]);
	FLASH_CS_ENABLE();
    HAL_SPI_TransmitReceive(spiflash->phspi, &spiflash->current_flash->write_cmd, &tmp, 1, TIMEOUT);
    HAL_SPI_TransmitReceive(spiflash->phspi, &array[1], &array[1], 3, TIMEOUT);
    HAL_SPI_TransmitReceive(spiflash->phspi, pbuffer, pbuffer, num_of_bytes, TIMEOUT);
	
	FLASH_CS_DISABLE();
	
    /* finished */
    spiflash_wait_op_finish();
	return num_of_bytes;
}
 
/**
 * @brief spi flash get sector offset
 */
static bool spiflash_get_page_start2end (uint32_t flash_addr, uint32_t *psurplus_of_bytes, uint32_t *pstart_align_addr, uint32_t *pend_align_addr)
{
	uint32_t temp_addr;
	if( !pstart_align_addr && !pend_align_addr ) return false;
	/* get first sector size */
	temp_addr = flash_addr % spiflash->current_flash->pagesize;
	temp_addr = spiflash->current_flash->pagesize - temp_addr;
	/* get flash_addr start position */
	*pstart_align_addr = flash_addr + temp_addr;
	temp_addr = *psurplus_of_bytes + flash_addr; 
	/* get psurplus of bytes */
	*psurplus_of_bytes = temp_addr % spiflash->current_flash->pagesize;
	/* get flash_addr end position */
	*pend_align_addr = temp_addr - ( *psurplus_of_bytes);
	return true;
}
/**
 * @brief spi flash write buffer
 */
uint32_t spiflash_write_buffer (uint32_t flash_addr, uint8_t *pbuffer, uint32_t num_of_bytes)
{
	if( !num_of_bytes ) return false;
	uint32_t start_addr, end_addr;
	uint32_t buf_index;
	spiflash_get_page_start2end(flash_addr, &num_of_bytes, &start_addr, &end_addr);
	buf_index = start_addr - flash_addr;
	spiflash_write_a_page(flash_addr, pbuffer, buf_index);
	if( start_addr <= end_addr ) { /* if start <= end */
		while( start_addr < end_addr ) {
			spiflash_write_a_page(start_addr, &pbuffer[buf_index], spiflash->current_flash->pagesize);
			start_addr += spiflash->current_flash->pagesize;
			buf_index += spiflash->current_flash->pagesize;
		}
		spiflash_write_a_page(end_addr, &pbuffer[buf_index], num_of_bytes);
	}
	return true;
}
/**
 * @brief spi flash read buffer
 */
uint32_t spiflash_read_buffer (uint32_t flash_addr, uint8_t *pbuffer, uint32_t num_of_bytes)
{
	uint8_t array[4], *parray = (uint8_t *)&flash_addr;
	uint8_t tmp;
	/* write enable */
    spiflash_write_enable();
	array[0] = parray[3]; array[1] = parray[2]; array[2] = parray[1]; array[3] = parray[0];
	printf("%s -->0x%x\n", __func__, array[1]);
	printf("%s -->0x%x\n", __func__, array[2]);
	printf("%s -->0x%x\n", __func__, array[3]);
	FLASH_CS_ENABLE();
    HAL_SPI_TransmitReceive(spiflash->phspi, &spiflash->current_flash->read_cmd, &tmp, 1, TIMEOUT);
    HAL_SPI_TransmitReceive(spiflash->phspi, &array[1], &array[1], 3, TIMEOUT);
    HAL_SPI_TransmitReceive(spiflash->phspi, pbuffer, pbuffer, num_of_bytes, TIMEOUT);
	
	FLASH_CS_DISABLE();
	
	return num_of_bytes;
}
 
 
 
 
 