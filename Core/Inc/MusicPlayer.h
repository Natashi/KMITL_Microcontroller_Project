/*
 * MusicPlayer.h
 *
 *  Created on: 19 Nov 2022
 *      Author: Natashi
 */

#ifndef INC_MUSICPLAYER_H_
#define INC_MUSICPLAYER_H_

#include <stdbool.h>

#include "Synthesizer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPLAYER_PLAYING		0
#define MPLAYER_PAUSED		1
#define MPLAYER_STOPPED		2

#define MAX_SONG_SIZE		0x8000UL - 4UL		//32Kib / song
#define MAX_SONG_COUNT		10

#define SONG_ADDR_0			0x08010000UL
#define SONG_ADDR_1			0x08018000UL
#define SONG_ADDR_2			0x08020000UL
#define SONG_ADDR_3			0x08040000UL
#define SONG_ADDR_4			0x08080000UL
#define SONG_ADDR_5			0x080C0000UL
#define SONG_ADDR_6			0x08100000UL
#define SONG_ADDR_7			0x08140000UL
#define SONG_ADDR_8			0x08180000UL
#define SONG_ADDR_9			0x081C0000UL

typedef struct {
	WaveDescriptor waveDesc;
	Synthesizer synth;

	int songIndex;

	bool bRecording;
	bool bEnableSave;

	int playerState;

	float volumeRate;

	//-------------------------------------

	uint32_t* dacBuffer;
	uint32_t* dacBuffer2;
	uint32_t iDac;
	bool bNeedNewBuf;

	uint32_t recordingSizeTotal;
	uint32_t recordingSizeCurrent;
	byte* recordingBuffer;
	uint32_t iRecord;

	uint32_t inputRead;
	uint32_t playbackPos;

	byte tmpRecordBuffer[MAX_SONG_SIZE];
} MusicPlayer;

void MusicPlayer_New(MusicPlayer* player);

void MusicPlayer_SetIsRecording(MusicPlayer* player, bool bRecording);
void MusicPlayer_SetVolumeRate(MusicPlayer* player, float volume);

void MusicPlayer_Start(MusicPlayer* player, int playIndex);
void MusicPlayer_Pause(MusicPlayer* player);
void MusicPlayer_Stop(MusicPlayer* player);

int MusicPlayer_IsPlayback(MusicPlayer* player);

uint32_t MusicPlayer_GetSongAddr(int index);
bool MusicPlayer_IsSongSlotFilled(int index);

void MusicPlayer_ReadRecording(MusicPlayer* player);
uint32_t MusicPlayer_FlushRecording(MusicPlayer* player);

void MusicPlayer_PopulateBuffer(MusicPlayer* player);

void MusicPlayer_IT_LoadKeys(MusicPlayer* player);
void MusicPlayer_IT_RecordData(MusicPlayer* player);
void MusicPlayer_IT_SendPCMToDAC(MusicPlayer* player);

#ifdef __cplusplus
}
#endif

#endif /* INC_MUSICPLAYER_H_ */
