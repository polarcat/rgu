#pragma once

#ifdef __ANDROID__
char *engine_shared_dir(const char *appname);
void engine_open_uri(const char *uri);
void engine_delete_app(const char *uri);
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
static inline char *engine_shared_dir(const char *appname)
{
#define FREE_GAME_PATH "/tmp/rgs.quanta.free"
	struct stat st;
	if (stat(FREE_GAME_PATH, &st) < 0)
		return NULL;
	else if ((st.st_mode & S_IFMT) != S_IFDIR)
		return NULL;
	return strdup(FREE_GAME_PATH);
#undef FREE_GAME_PATH
}
#define engine_open_uri(uri) ;
#define engine_delete_app(uri) ;
#endif
