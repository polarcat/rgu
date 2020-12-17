/* audio.h: audio interface
 *
 * Copyright (C) 2020 by Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

struct player;
struct engine;

struct volume {
	int16_t val;
	int16_t max;
};

struct track {
	const char *name;
	uint8_t loaded:1;
	const uint8_t keep:1;
	struct volume volume;
	struct player *player;
	struct engine *engine; /* engine binding */
};

struct engine *audio_open(void);
void audio_close(struct engine **);
void audio_load(void *amgr, struct track *);
void audio_unload(struct track *);
void audio_loop(struct track *, uint8_t loop);
void audio_play(struct track *);
void audio_stop(struct track *);
uint8_t audio_fade(struct track *);
void audio_pause(struct track *);
float audio_progress(struct track *);
