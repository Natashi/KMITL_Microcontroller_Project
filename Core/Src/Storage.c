/*
 * Storage.c
 *
 *  Created on: 19 Nov 2022
 *      Author: Natashi
 */

#include "Storage.h"
#include "string.h"

FLASH_EraseInitTypeDef hFlashEraseInit;

int StorageFlash_GetSector(uint32_t startAddr) {
	if (startAddr < FLASHRGN_START || startAddr > FLASHRGN_END)
		return -1;

	//32KB sectors
	if (startAddr < 0x08008000)
		return 0;
	else if (startAddr < 0x08010000)
		return 1;
	else if (startAddr < 0x08018000)
		return 2;
	else if (startAddr < 0x08020000)
		return 3;

	//128KB sector
	else if (startAddr < 0x08040000)
		return 4;

	//256KB sectors
	else if (startAddr < 0x08080000)
		return 5;
	else if (startAddr < 0x080C0000)
		return 6;
	else if (startAddr < 0x08100000)
		return 7;
	else if (startAddr < 0x08140000)
		return 8;
	else if (startAddr < 0x08180000)
		return 9;
	else if (startAddr < 0x081C0000)
		return 10;

	return 11;
}

void StorageFlash_Init() {
	{
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();

		while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET);
		FLASH->OPTCR &= ~FLASH_OPTCR_nDBANK;

		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();

		while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET);
	}

	hFlashEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
	hFlashEraseInit.Sector = 0;
	hFlashEraseInit.NbSectors = 1;
	hFlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
}

int StorageFlash_Erase(uint32_t startAddr) {
	if (startAddr < FLASHRGN_START)
		return -1;

	HAL_FLASH_Unlock();

	hFlashEraseInit.Sector = StorageFlash_GetSector(startAddr);
	hFlashEraseInit.NbSectors = 1;

	HAL_StatusTypeDef res;
	uint32_t sectorErr;
	if ((res = HAL_FLASHEx_Erase(&hFlashEraseInit, &sectorErr)) != HAL_OK)
		return sectorErr;
	FLASH_WaitForLastOperation(1000);

	HAL_FLASH_Lock();
	return 0;
}

int StorageFlash_Write(uint32_t startAddr, uint32_t size, const uint32_t* data) {
	if (startAddr < FLASHRGN_START || (startAddr + size > FLASHRGN_END))
		return -1;

	int res = StorageFlash_Erase(startAddr);
	if (res != 0)
		return res;

	HAL_FLASH_Unlock();

	uint32_t nWords = size / sizeof(uint32_t);
	uint32_t addr = startAddr;
	for (uint32_t iWord = 0; iWord < nWords; ++iWord) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data[iWord]) != HAL_OK)
			return HAL_FLASH_GetError();
		FLASH_WaitForLastOperation(1000);
		addr += sizeof(uint32_t);
	}

	HAL_FLASH_Lock();
	return 0;
}
int StorageFlash_ReadDirect(uint32_t startAddr, __IO uint32_t** data) {
	if (startAddr < FLASHRGN_START)
		return -1;

	*data = (__IO uint32_t*)startAddr;

	return 0;
}
int StorageFlash_Read(uint32_t startAddr, uint32_t size, uint32_t* data) {
	if (startAddr < FLASHRGN_START || (startAddr + size > FLASHRGN_END))
		return -1;

	uint32_t nWords = size / sizeof(uint32_t);
	uint32_t addr = startAddr;
	for (uint32_t iWord = 0; iWord < nWords; ++iWord) {
		data[iWord] = *(__IO uint32_t *)addr;
		addr += sizeof(uint32_t);
	}

	return 0;
}
