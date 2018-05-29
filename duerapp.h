
#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_H

#include "duerapp_config.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define RESOURCE_PATH		"/home/hujie/source/resources"
#define PROFILE_PATH		RESOURCE_PATH"/profile"
#define VOICE_PATH			RESOURCE_PATH

#define DEFAULT_ARGS	\
	{PROFILE_PATH, VOICE_PATH, 5}

typedef struct
{
	char *profile;
	char *voice_record;
	int  interval;
} duerapp_args_t;

#ifdef __cplusplus
}
#endif

#endif
