/* audio.c: audio utils
 *
 * Copyright (C) 2020 by Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#ifdef ANDROID

#include <stdlib.h>
#include <android/asset_manager_jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define TAG "audio"

#include <rgu/log.h>
#include <rgu/audio.h>

struct player {
	SLObjectItf obj;
	SLPlayItf play;
	SLSeekItf seek;
};

struct engine {
	SLObjectItf obj;
	SLEngineItf iface;
	SLObjectItf output;
};

static inline uint8_t get_state(struct track *track)
{
	SLuint32 state;
	(*track->player->play)->GetPlayState(track->player->play, &state);

	return state;
}

float audio_progress(struct track *track)
{
	if (!track->player || !track->player->play)
		return -1;
	else if (get_state(track) != SL_PLAYSTATE_PLAYING)
		return -1;

	struct player *player = track->player;
	SLmillisecond pos;
	(*player->play)->GetPosition(player->play, &pos);

	SLmillisecond dur;
	(*player->play)->GetDuration(player->play, &dur);
	if (dur == UINT32_MAX)
		return 0;

	return (float) pos / dur;
}

void audio_unload(struct track *track)
{
	if (!track->player)
		return;

	if (track->player->obj)
		(*track->player->obj)->Destroy(track->player->obj);

	track->player->play = NULL;
	track->player->seek = NULL;
	track->engine = NULL;
	free(track->player);
	track->player = NULL;
	track->loaded = 0;
}

void audio_load(void *amgr, struct track *track)
{
	if (!track->engine) {
		ee("track '%s' is not associated with audio engine\n",
		  track->name);
		return;
	}

	if (!(track->player = calloc(1, sizeof(*track->player)))) {
		ee("failed to allocate %zu bytes\n", sizeof(*track->player));
		return;
	}

	AAsset *asset = AAssetManager_open(amgr, track->name,
	  AASSET_MODE_UNKNOWN);

	if (!asset) {
		ee("failed to load asset '%s'\n", track->name);
		return;
	}

	off_t start, len;
	int fd = AAsset_openFileDescriptor(asset, &start, &len);

	if (fd <= 0) {
		ee("failed to open asset file '%s'\n", track->name);
		AAsset_close(asset);
		return;
	}

	AAsset_close(asset);

	SLDataLocator_AndroidFD slfd = {
		SL_DATALOCATOR_ANDROIDFD, fd, start, len
	};
	SLDataFormat_MIME format_mime = {
		SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED
	};
	SLDataSource src = { &slfd, &format_mime };
	SLDataLocator_OutputMix output = {
		SL_DATALOCATOR_OUTPUTMIX, track->engine->output
	};
	SLDataSink sink = { &output, NULL };
	const SLInterfaceID ids[3] = {
		SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME
	};
	const SLboolean req[3] = {
		SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
	};

	struct engine *engine = track->engine;
	struct player *player = track->player;

	SLresult res = (*engine->iface)->CreateAudioPlayer(engine->iface,
	  &player->obj, &src, &sink, 3, ids, req);
	if (!*player->obj || SL_RESULT_SUCCESS != res) {
		ee("failed to create audio track object for '%s'\n",
		  track->name);
		return;
	}

	res = (*player->obj)->Realize(player->obj, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to init audio track object for '%s'\n",
		  track->name);
		return;
	}

	res = (*player->obj)->GetInterface(player->obj, SL_IID_PLAY,
	  &player->play);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to get play interface for '%s'\n", track->name);
		return;
	}

	res = (*player->obj)->GetInterface(player->obj, SL_IID_SEEK,
	  &player->seek);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to get seek interface for '%s'\n", track->name);
		return;
	}

	track->loaded = 1;
}

void audio_loop(struct track *track, uint8_t loop)
{
	if (!track->player || !track->player->seek) {
		ee("failed to set loop for sound '%s' (no seek)\n",
		  track->name);
		return;
	}

	struct player *player = track->player;
	SLresult res = (*player->seek)->SetLoop(player->seek,
	  loop ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE, 0, SL_TIME_UNKNOWN);

	if (SL_RESULT_SUCCESS != res) {
		ee("failed to set loop for sound '%s'\n", track->name);
		return;
	}
}

void audio_pause(struct track *track)
{
	if (!track->player || !track->player->play)
		return;
	else if (get_state(track) == SL_PLAYSTATE_PAUSED)
		return;

	struct player *player = track->player;
	(*player->play)->SetPlayState(player->play, SL_PLAYSTATE_PAUSED);
}

void audio_play(struct track *track)
{
	if (!track->player || !track->player->play)
		return;
	else if (get_state(track) == SL_PLAYSTATE_PLAYING)
		return;

	struct player *player = track->player;
	(*player->play)->SetPlayState(player->play, SL_PLAYSTATE_PLAYING);
}

void audio_stop(struct track *track)
{
	if (!track->player || !track->player->play)
		return;
	else if (get_state(track) == SL_PLAYSTATE_STOPPED)
		return;

	struct player *player = track->player;
	(*player->play)->SetPlayState(player->play, SL_PLAYSTATE_STOPPED);
}

void audio_close(struct engine **engine)
{
	if (!*engine)
		return;

	if ((*engine)->output)
		(*(*engine)->output)->Destroy((*engine)->output);

	if ((*engine)->obj)
		(*(*engine)->obj)->Destroy((*engine)->obj);

	free(*engine);
	*engine = NULL;
}

struct engine *audio_open(void)
{
	struct engine *engine = calloc(1, sizeof(*engine));

	if (!engine) {
		ee("failed to allocate %zu bytes\n", sizeof(*engine));
		return NULL;
	}

	SLresult res = slCreateEngine(&engine->obj, 0, NULL, 0, NULL, NULL);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to create audio engine object\n");
		goto err;
	}

	res = (*engine->obj)->Realize(engine->obj, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to init audio engine object\n");
		goto err;
	}

	res = (*engine->obj)->GetInterface(engine->obj, SL_IID_ENGINE,
	  &engine->iface);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to get audio engine interface\n");
		goto err;
	}

	/* disable REVERB feature */
	const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[1] = {SL_BOOLEAN_FALSE};

	res = (*engine->iface)->CreateOutputMix(engine->iface, &engine->output,
	  1, ids, req);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to create output mixer\n");
		goto err;
	}

	res = (*engine->output)->Realize(engine->output, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to init output mixer\n");
		goto err;
	}

	ii("audio interface is ready\n");
	return engine;
err:
	audio_close(&engine);
	return NULL;
}

#endif /* ANDROID */
