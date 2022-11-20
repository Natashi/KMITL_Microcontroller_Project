/*
 * Synthesizer.h
 *
 *  Created on: Nov 14, 2022
 *      Author: Natashi
 */

#ifndef INC_SYNTHESIZER_H_
#define INC_SYNTHESIZER_H_

#include "WaveDescriptor.h"
#include "wave_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t key_;
	float frequency_;
	float phase_;
	float amplitude_;
} Note;
Note Note_New(uint16_t key, float frequency, float amplitudeMul);

typedef struct {
	WaveDescriptor* pWaveDescriptor;

	bool keyStates[7];
	Note keys[7];

	uint32_t* pcmBuffer;
	uint32_t pcmIndex;
} Synthesizer;
void Synthesizer_New(Synthesizer* synth, WaveDescriptor* waveDescriptor);

void Synthesizer_SynthesizeFrame(Synthesizer* synth, byte input, float pcmMul);
int Synthesizer_LoadPCM(Synthesizer* synth, byte* inputData, float volume, uint32_t* out);

#ifdef __cplusplus
}
#endif

#endif /* INC_SYNTHESIZER_H_ */
