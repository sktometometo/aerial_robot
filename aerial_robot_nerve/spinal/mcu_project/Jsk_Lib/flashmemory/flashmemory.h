/*
 * flashmemory.h
 *
 *  Created on: 2016/11/18
 *      Author: anzai
 */

#ifndef APPLICATION_JSK_LIB_FLASHMEMORY_FLASHMEMORY_H_
#define APPLICATION_JSK_LIB_FLASHMEMORY_FLASHMEMORY_H_

#include <vector>
#include "stm32f7xx_hal.h"

namespace FlashMemory {
	void init(uint32_t data_address, uint32_t data_sector);
	void addValue(void* ptr, size_t size);
	HAL_StatusTypeDef read();
	void erase();
	void write();
}




#endif /* APPLICATION_JSK_LIB_FLASHMEMORY_FLASHMEMORY_H_ */
