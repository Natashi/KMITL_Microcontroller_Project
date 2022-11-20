/*
 * Storage.h
 *
 *  Created on: 19 Nov 2022
 *      Author: Natashi
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include "main.h"

extern FLASH_EraseInitTypeDef hFlashEraseInit;

#define FLASHRGN_START 	0x08000000UL
#define FLASHRGN_END 	0x081FFFFFUL

int StorageFlash_GetSector(uint32_t startAddr);

void StorageFlash_Init();
int StorageFlash_Erase(uint32_t startAddr);
int StorageFlash_Write(uint32_t startAddr, uint32_t size, const uint32_t* data);
int StorageFlash_ReadDirect(uint32_t startAddr, __IO uint32_t** data);
int StorageFlash_Read(uint32_t startAddr, uint32_t size, uint32_t* data);

#endif /* INC_STORAGE_H_ */
