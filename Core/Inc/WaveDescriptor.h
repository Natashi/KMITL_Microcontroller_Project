/*
 * WaveDescriptor.hpp
 *
 *  Created on: Nov 16, 2022
 *      Author: Natashi
 */

#ifndef INC_WAVEDESCRIPTOR_HPP_
#define INC_WAVEDESCRIPTOR_HPP_

#include <stdint.h>
typedef unsigned char byte;

#include <stdlib.h>
#include <string.h>

#include "main.h"

typedef struct {
	uint32_t sampleRate;
	uint32_t frameBufferSize;
} WaveDescriptor;

typedef struct {
	uint32_t frameCount;
	byte* frames;
} ShittyAudioFormat;

#endif /* INC_WAVEDESCRIPTOR_HPP_ */
