
#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_EVENT_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_HUJIE_DEMO_DUERAPP_EVENT_H

#include "duerapp_config.h"

#ifdef _cplusplus
extern "C" {
#endif

enum
{
	KBDEVT_RECORD = 'r',
	KBDEVT_QUIT   = 'q',
};

void duerapp_event_loop(void);

#ifdef _cplusplus
}
#endif

#endif
