/* audio.c: audio utils
 *
 * Copyright (C) 2020 by Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#include <android/asset_manager_jni.h>

#define TAG "audio"

#include <rgu/log.h>
#include <rgu/audio.h>

static SLObjectItf engine_obj_ = NULL;
static SLEngineItf engine_;
static SLObjectItf output_obj_ = NULL;
static SLObjectItf track_obj_ = NULL;

static inline uint8_t get_state(struct track *track)
{
	SLuint32 state;
	(*track->play)->GetPlayState(track->play, &state);

	return state;
}

float audio_progress(struct track *track)
{
	if (!track->play)
		return -1;
	else if (get_state(track) != SL_PLAYSTATE_PLAYING)
		return -1;

	SLmillisecond pos;
	(*track->play)->GetPosition(track->play, &pos);

	SLmillisecond dur;
	(*track->play)->GetDuration(track->play, &dur);
	if (dur == UINT32_MAX)
		return 0;

	return (float) pos / dur;
}

void audio_load(void *amgr, struct track *track)
{
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
		SL_DATALOCATOR_OUTPUTMIX, output_obj_
	};
	SLDataSink sink = { &output, NULL };
	const SLInterfaceID ids[3] = {
		SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME
	};
	const SLboolean req[3] = {
		SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
	};

	SLresult res = (*engine_)->CreateAudioPlayer(engine_, &track_obj_,
	  &src, &sink, 3, ids, req);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to create audio track object for '%s'\n",
		  track->name);
		return;
	}

	res = (*track_obj_)->Realize(track_obj_, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to init audio track object for '%s'\n",
		  track->name);
		return;
	}

	res = (*track_obj_)->GetInterface(track_obj_, SL_IID_PLAY,
	  &track->play);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to get play interface for '%s'\n", track->name);
		return;
	}

	res = (*track_obj_)->GetInterface(track_obj_, SL_IID_SEEK,
	  &track->seek);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to get seek interface for '%s'\n", track->name);
		return;
	}
}

void audio_loop(struct track *track, uint8_t loop)
{
	if (!track->seek) {
		ee("failed to set loop for sound '%s' (no seek)\n",
		  track->name);
		return;
	}

	SLresult res = (*track->seek)->SetLoop(track->seek,
	  loop ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE, 0, SL_TIME_UNKNOWN);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to set loop for sound '%s'\n", track->name);
		return;
	}
}

void audio_pause(struct track *track)
{
	if (!track->play)
		return;
	else if (get_state(track) == SL_PLAYSTATE_PAUSED)
		return;

	(*track->play)->SetPlayState(track->play, SL_PLAYSTATE_PAUSED);
}

void audio_play(struct track *track)
{
	if (!track->play)
		return;
	else if (get_state(track) == SL_PLAYSTATE_PLAYING)
		return;

	(*track->play)->SetPlayState(track->play, SL_PLAYSTATE_PLAYING);
}

void audio_stop(struct track *track)
{
	if (!track->play)
		return;
	else if (get_state(track) == SL_PLAYSTATE_STOPPED)
		return;

	(*track->play)->SetPlayState(track->play, SL_PLAYSTATE_STOPPED);
}

void audio_close(void)
{
	if (track_obj_)
		(*track_obj_)->Destroy(track_obj_);

	if (output_obj_)
		(*output_obj_)->Destroy(output_obj_);

	if (engine_obj_)
		(*engine_obj_)->Destroy(engine_obj_);
}

void audio_open(void)
{
	SLresult res = slCreateEngine(&engine_obj_, 0, NULL, 0, NULL, NULL);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to create audio engine object\n");
		return;
	}

	res = (*engine_obj_)->Realize(engine_obj_, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to init audio engine object\n");
		return;
	}

	res = (*engine_obj_)->GetInterface(engine_obj_, SL_IID_ENGINE,
	  &engine_);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to get audio engine interface\n");
		return;
	}

	/* disable REVERB feature */
	const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[1] = {SL_BOOLEAN_FALSE};

	res = (*engine_)->CreateOutputMix(engine_, &output_obj_, 1, ids, req);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to create output mixer\n");
		return;
	}

	res = (*output_obj_)->Realize(output_obj_, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != res) {
		ee("failed to init output mixer\n");
		return;
	}

	ii("audio interface is ready\n");
}
