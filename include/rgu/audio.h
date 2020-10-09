/* audio.h: audio interface
 *
 * Copyright (C) 2020 by Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#ifdef ANDROID
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#endif

enum {
	AUDIO_STATUS_STOPPED,
	AUDIO_STATUS_STARTED,
	AUDIO_STATUS_PLAYING,
};

struct track {
	uint8_t ready:1;
	const char *name;
	uint8_t status;
	int32_t counter;
#ifdef ANDROID
	SLPlayItf play;
	SLSeekItf seek;
#else
	void *play;
	void *seek;
#endif
};

void audio_open(void);
void audio_close(void);
void audio_load(void *amgr, struct track *);
void audio_loop(struct track *, uint8_t loop);
void audio_play(struct track *);
void audio_stop(struct track *);
void audio_pause(struct track *);
float audio_progress(struct track *);
