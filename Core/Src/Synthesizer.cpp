/*
 * Synthesizer.cpp
 *
 *  Created on: Nov 14, 2022
 *      Author: Natashi
 */

#include "Synthesizer.h"

#include "stm32f7xx_hal.h"
#include "math.h"

Note Note_New(uint16_t key, float frequency, float amplitudeMul) {
	Note note;
	note.key_ = key;
	note.frequency_ = frequency;
	note.phase_ = 0;
	note.amplitude_ = amplitudeMul;
	return note;
}

void Synthesizer_New(Synthesizer* synth, WaveDescriptor* waveDescriptor) {
	synth->pWaveDescriptor = waveDescriptor;

	synth->keys[0] = Note_New(GPIO_PIN_0, 261.63f, 1.0f);	//c
	synth->keys[1] = Note_New(GPIO_PIN_1, 293.66f, 0.903f);	//d
	synth->keys[2] = Note_New(GPIO_PIN_2, 329.63f, 0.819f);	//e
	synth->keys[3] = Note_New(GPIO_PIN_3, 349.23f, 0.741f);	//f
	synth->keys[4] = Note_New(GPIO_PIN_4, 392.00f, 0.67f);	//g
	synth->keys[5] = Note_New(GPIO_PIN_5, 440.00f, 0.607f);	//a
	synth->keys[6] = Note_New(GPIO_PIN_6, 493.88f, 0.549f);	//b

	synth->pcmBuffer = new uint32_t[waveDescriptor->frameBufferSize];
	synth->pcmIndex = 0;
}

void Synthesizer_SynthesizeFrame(Synthesizer* synth, byte input, float pcmMul) {
	constexpr float PI_X2 = 2 * 3.141593f;

	float sineAccum = 0;

	for (int i = 0; i < 7; ++i) {
		Note* pNote = &synth->keys[i];

		if ((input >> i) & 1) {
			pNote->phase_ += PI_X2 * pNote->frequency_ / synth->pWaveDescriptor->sampleRate;

			while (pNote->phase_ >= PI_X2)
				pNote->phase_ -= PI_X2;
			while (pNote->phase_ < 0)
				pNote->phase_ += PI_X2;

			//sineAccum += GetSine((int)(pNote->phase_ * 360));
			sineAccum += sin(pNote->phase_);
		}
	}

	sineAccum *= pcmMul;

	int pcm = 2048 + sineAccum * 2000;
	if (pcm < 0) pcm = 0;
	if (pcm > 0xfff) pcm = 0xfff;

	uint32_t idx = synth->pcmIndex;
	synth->pcmBuffer[idx] = pcm;

	++idx;
	if (idx > synth->pWaveDescriptor->frameBufferSize)
		idx = 0;

	synth->pcmIndex = idx;
}

static int _GetBitCount(byte b) {
	int res = 0;
	while (b) {
		res += b & 1;
		b >>= 1;
	}
	return res;
}

int Synthesizer_LoadPCM(Synthesizer* synth, byte* inputData, float volume, uint32_t* out) {
	const float INV_TABLE[] = {
		1.0f, 0.5f, 0.3333f, 0.25f, 0.2f, 0.1667f, 0.1429f, 0.125f
	};


	//1 input lasts for 		1000/20  	= 50ms
	//1 pcm sample lasts for 	1000/10000 	= 0.1ms
	//1 input must last for 	50/0.1		= 500 pcm samples
	//	round up to 512 since 4096 % 512 = 0

	uint32_t bufSize = synth->pWaveDescriptor->frameBufferSize;
	uint32_t iInput = 0;

#define LOAD_MULT countActive = _GetBitCount(inputData[iInput]); \
	mult = volume * INV_TABLE[countActive]; \
	if (mult > 1) mult = 1; \
	else if (mult < 0) mult = 0;

	int countActive; float mult;
	LOAD_MULT;

	synth->pcmIndex = 0;
	for (uint32_t i = 0, j = 0; i < bufSize; i++) {
		Synthesizer_SynthesizeFrame(synth, inputData[iInput], mult);

		j++;
		if (j == 512) {
			++iInput;
			LOAD_MULT;
		}
	}

#undef LOAD_MULT

	return PCM_FRAMEBUFFER_SIZE / 512;

	//memcpy(&out[0], synth->pcmBuffer, sizeof(uint32_t) * bufSize);

	/*
	memcpy(&out[0], SINE_TABLE_UINT, sizeof(uint32_t) * 256);
	memcpy(&out[256], SINE_TABLE_UINT, sizeof(uint32_t) * 256);
	memcpy(&out[512], SINE_TABLE_UINT, sizeof(uint32_t) * 256);
	memcpy(&out[768], SINE_TABLE_UINT, sizeof(uint32_t) * 256);
	*/
}
