#pragma once

#ifdef __ANDROID__
char *engine_shared_dir(const char *appname);
void engine_open_uri(const char *uri);
void engine_delete_app(const char *uri);
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <game/game.h>

static inline char *engine_shared_dir(const char *appname)
{
	struct stat st;
	if (stat(SYNC_DIR, &st) < 0)
		return NULL;
	else if ((st.st_mode & S_IFMT) != S_IFDIR)
		return NULL;
	return strdup(SYNC_DIR);
}
#define engine_open_uri(uri) ;
#define engine_delete_app(uri) ;
#endif
