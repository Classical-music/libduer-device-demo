
#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_RECORD_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_RECORD_H

#include <alsa/asoundlib.h>

#include "duerapp_config.h"

#ifdef _cplusplus
extern "C" {
#endif

enum
{
	RECORD_CAPTURE,
	RECORD_UNCAPTURE,
	RECORD_QUIT,
};

typedef struct
{
	int state;
	unsigned int rate;
	snd_pcm_uframes_t frames;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	pthread_t thread_id;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} duerapp_record_config_t;

void duerapp_record_init(void);

void duerapp_record_destory(void);

void duerapp_record_capture_onoff(void);

#ifdef _cplusplus
}
#endif

#endif
