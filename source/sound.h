#pragma once

#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    u8* sndbuffer;
    u32 sndsize;
    int channel;
    int duration;
	int level;
	bool loaded;
} Track;

Track g_hexagon;
Track g_select;
Track g_begin;
Track g_over;
Track g_bgm;

bool audioPlay(Track *sound, bool loop);
void audioUnload();
void initSounds();
void playLevelBGM(int level);