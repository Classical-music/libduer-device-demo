
#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_MEDIA_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_MEDIA_H

#include <gst/gst.h>
#include "duerapp_config.h"

#ifdef _cplusplus
extern "C" {
#endif

enum
{
	MEDIA_PLAY,
	MEDIA_PAUSE,
	MEDIA_STOP,
	MEDIA_QUIT,
};

typedef struct
{
	int state;
	const char *url;
	bool mute;
	float volume;
	GMainLoop *loop;
	GstElement *pipeline;
	guint bus_watch_id;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t thread_id;
	void (*duerapp_media_player_finished)(void);
} duerapp_media_config_t;

void duerapp_media_init(void);

void duerapp_media_destory(void);

void duerapp_media_speak_player(const char *);

void duerapp_media_speak_stop(void);

void duerapp_media_audio_player(const char *);

void duerapp_media_audio_pause(void);

void duerapp_media_audio_resume(void);

void duerapp_media_audio_stop(void);

#ifdef _cplusplus
}
#endif

#endif
