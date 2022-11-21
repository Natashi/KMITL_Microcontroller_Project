/*
 * MusicPlayer.c
 *
 *  Created on: Nov 19, 2022
 *      Author: Natashi
 */

#include "MusicPlayer.h"
#include "Storage.h"

#include "dac.h"
#include "tim.h"

//I'm too fucking tired to clean this up, so deal with it

void MusicPlayer_New(MusicPlayer* player) {
	player->waveDesc.sampleRate = 14000;
	player->waveDesc.frameBufferSize = PCM_FRAMEBUFFER_SIZE;

	Synthesizer_New(&player->synth, &player->waveDesc);

	player->songIndex = 1;
	player->bRecording = false;
	player->playerState = MPLAYER_STOPPED;
	player->volumeRate = 1.0f;

	uint32_t dacBufSize = player->waveDesc.frameBufferSize * sizeof(uint32_t);
	player->dacBuffer = malloc(dacBufSize);
	//player->dacBuffer2 = malloc(dacBufSize);
	player->iDac = 0;
	player->bNeedNewBuf = false;

	player->recordingSizeTotal = 0;
	player->recordingSizeCurrent = 0;
	player->recordingBuffer = malloc((1000 / TIMERFREQ_LOADKEYS + 10) * sizeof(byte));

	player->inputRead = 0;
}

void MusicPlayer_SetIsRecording(MusicPlayer* player, bool bRecording) {
	player->bRecording = bRecording;
}
void MusicPlayer_SetVolumeRate(MusicPlayer* player, float volume) {
	player->volumeRate = volume;
}

void MusicPlayer_Start(MusicPlayer* player, int playIndex) {
	//Initialize fields

	player->songIndex = playIndex;

	player->bEnableSave = true;

	player->iDac = 0;

	uint32_t frameBufSize = player->waveDesc.frameBufferSize * sizeof(uint32_t);
	memset(player->synth.pcmBuffer, 0, frameBufSize);
	memset(player->dacBuffer, 0, frameBufSize);

	player->bNeedNewBuf = false;

	player->recordingSizeTotal = 0;
	player->recordingSizeCurrent = 0;
	if (player->bRecording) {
		player->recordingSizeTotal += 4;
		*(uint32_t*)player->tmpRecordBuffer = 0;
	}
	else {
		MusicPlayer_ReadRecording(player);
	}

	player->inputRead = 0;
	player->playbackPos = 4;

	//Open the file to store the recording
		//We're using flash, so no need to do anything special

	//Start da timers

	if (player->playerState == MPLAYER_STOPPED) {
		__HAL_TIM_SET_COUNTER(TIMER_SENDPCM, 0);
		__HAL_TIM_SET_COUNTER(TIMER_LOADKEYS, 0);
		__HAL_TIM_SET_COUNTER(TIMER_WRITEDATA, 0);
	}
	HAL_TIM_Base_Start_IT(TIMER_SENDPCM);
	HAL_TIM_Base_Start_IT(TIMER_LOADKEYS);
	HAL_TIM_Base_Start_IT(TIMER_WRITEDATA);

	player->playerState = MPLAYER_PLAYING;
}
void MusicPlayer_Pause(MusicPlayer* player) {
	if (player->playerState == MPLAYER_PLAYING) {
		player->playerState = MPLAYER_PAUSED;

		HAL_TIM_Base_Stop(TIMER_SENDPCM);
		HAL_TIM_Base_Stop(TIMER_LOADKEYS);
	}
	else if (player->playerState == MPLAYER_PAUSED) {
		player->playerState = MPLAYER_PLAYING;

		HAL_TIM_Base_Start_IT(TIMER_SENDPCM);
		HAL_TIM_Base_Start_IT(TIMER_LOADKEYS);
	}
}
void MusicPlayer_Stop(MusicPlayer* player) {
	if (player->playerState != MPLAYER_STOPPED) {
		HAL_TIM_Base_Stop(TIMER_SENDPCM);
		HAL_TIM_Base_Stop(TIMER_LOADKEYS);

		__HAL_TIM_SET_COUNTER(TIMER_SENDPCM, 0);
		__HAL_TIM_SET_COUNTER(TIMER_LOADKEYS, 0);
		//__HAL_TIM_SET_COUNTER(TIMER_WRITEDATA, 0);

		player->playerState = MPLAYER_STOPPED;
	}
}

int MusicPlayer_IsPlayback(MusicPlayer* player) {
	return !player->bRecording && (player->playerState != MPLAYER_STOPPED);
}

uint32_t MusicPlayer_GetSongAddr(int index) {
	switch (index) {
	case 0: return SONG_ADDR_0;
	case 1: return SONG_ADDR_1;
	case 2: return SONG_ADDR_2;
	case 3: return SONG_ADDR_3;
	case 4: return SONG_ADDR_4;
	case 5: return SONG_ADDR_5;
	case 6: return SONG_ADDR_6;
	case 7: return SONG_ADDR_7;
	case 8: return SONG_ADDR_8;
	case 9: return SONG_ADDR_9;
	}
	return SONG_ADDR_0;
}
bool MusicPlayer_IsSongSlotFilled(int index) {
	uint32_t tmp;

	uint32_t addr = MusicPlayer_GetSongAddr(index);
	StorageFlash_Read(addr, sizeof(uint32_t), &tmp);

	return tmp != 0;
}

void MusicPlayer_ReadRecording(MusicPlayer* player) {
	uint32_t addr = MusicPlayer_GetSongAddr(player->songIndex);

	uint32_t totalSize;
	StorageFlash_Read(addr, 4, &totalSize);

	player->recordingSizeCurrent = 4;
	player->recordingSizeTotal = totalSize;

	StorageFlash_Read(addr, totalSize,
		(uint32_t*)player->tmpRecordBuffer);
	;
}
uint32_t MusicPlayer_FlushRecording(MusicPlayer* player) {
	if (!player->bRecording || !player->bEnableSave)
		return 0;
	while (player->recordingSizeCurrent > 0) HAL_Delay(500);

	HAL_TIM_Base_Stop(TIMER_WRITEDATA);
	__HAL_TIM_SET_COUNTER(TIMER_WRITEDATA, 0);

	if (player->recordingSizeTotal > MAX_SONG_SIZE)
		player->recordingSizeTotal = MAX_SONG_SIZE;

	while (player->recordingSizeTotal % 4 != 0) {
		player->tmpRecordBuffer[(player->recordingSizeTotal)++] = '\0';
	}

	*(uint32_t*)player->tmpRecordBuffer = player->recordingSizeTotal;

	uint32_t addr = MusicPlayer_GetSongAddr(0);
	StorageFlash_Write(addr, player->recordingSizeTotal, (const uint32_t*)player->tmpRecordBuffer);

	return player->recordingSizeTotal;
}

void MusicPlayer_PopulateBuffer(MusicPlayer* player) {
	if (!player->bRecording) {
		if (player->playbackPos >= player->recordingSizeTotal) {
			MusicPlayer_Stop(player);
			return;
		}
	}

	int advance = Synthesizer_LoadPCM(&player->synth,
		&player->recordingBuffer[player->inputRead],
		player->volumeRate, player->dacBuffer);
	player->inputRead += advance;

	/*
	//Wrap-around memcpy with offset
	uint32_t iDac = player->iDac;
	uint32_t len1 = player->waveDesc.frameBufferSize - iDac;
	memcpy(player->dacBuffer + iDac, player->dacBuffer2, len1 * sizeof(uint32_t));
	memcpy(player->dacBuffer, player->dacBuffer2 + len1, iDac * sizeof(uint32_t));
	*/

	player->bNeedNewBuf = false;
}

void MusicPlayer_IT_LoadKeys(MusicPlayer* player) {
	if (player->bRecording) {	//Free play
		uint32_t idr = GPIOD->IDR;

		for (int i = 0; i < 7; ++i) {
			Note* pNote = &player->synth.keys[i];

			player->synth.keyStates[i] = (idr & pNote->key_) == GPIO_PIN_RESET;
		}

		byte key = 0;
		for (int i = 0; i < 7; ++i) {
			key |= (byte)player->synth.keyStates[i] << i;
		}

		player->recordingBuffer[player->recordingSizeCurrent] = key;
		++(player->recordingSizeCurrent);
	}
	else {						//Read from recording
		byte key = 0;
		if (player->playbackPos < player->recordingSizeTotal) {
			key = player->tmpRecordBuffer[player->playbackPos];
		}
		player->recordingBuffer[player->recordingSizeCurrent] = key;
		++(player->recordingSizeCurrent);
		++(player->playbackPos);
	}

	player->inputRead = 0;
}
void MusicPlayer_IT_RecordData(MusicPlayer* player) {
	uint32_t thisRecord = player->recordingSizeCurrent;
	uint32_t total = player->recordingSizeTotal;
	if (player->bRecording && thisRecord > 0 && total < MAX_SONG_SIZE) {
		if (total + thisRecord > MAX_SONG_SIZE)
			thisRecord = total - MAX_SONG_SIZE;

		memcpy(player->tmpRecordBuffer + total,
			player->recordingBuffer, thisRecord);
		player->recordingSizeTotal += thisRecord;
	}
	player->recordingSizeCurrent = 0;
	player->inputRead = 0;
}
void MusicPlayer_IT_SendPCMToDAC(MusicPlayer* player) {
	if (player->playerState == MPLAYER_PLAYING) {
		HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R,
				player->synth.pcmBuffer[player->iDac]);

		++(player->iDac);
		if (player->iDac >= player->synth.pWaveDescriptor->frameBufferSize) {
			player->iDac = 0;
			MusicPlayer_PopulateBuffer(player);
			player->bNeedNewBuf = true;
		}
	}
	else {
		HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
	}
}
