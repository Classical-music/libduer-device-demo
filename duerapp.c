/**
 * Copyright (2018) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp.c
 * Auth: Hu Jie (v_hujie@baidu.com)
 * Desc: Duer Application Main.
 */

#include "duerapp.h"
#include "duerapp_event.h"
#include "duerapp_record.h"
#include "duerapp_media.h"

#include "lightduer_dcs.h"
#include "lightduer_connagent.h"
#include "lightduer_system_info.h"
//#include "lightduer_dev_info.h"

const duer_system_static_info_t g_system_static_info = {
    .os_version         = "Ubuntu 18.04.4",
    .sw_version         = "hujie-linux-demo",
    .brand              = "Baidu",
    .hardware_version   = "1.0.0",
    .equipment_type     = "PC",
    .ram_size           = 4096 * 1024,
    .rom_size           = 2048 * 1024,
};
static bool b_duer_start = false;

static void duerapp_dcs_init(void)
{
	duer_dcs_framework_init();
	duer_dcs_voice_input_init();
	duer_dcs_voice_output_init();
	duer_dcs_speaker_control_init();
	duer_dcs_audio_player_init();
	duer_dcs_screen_init();
}

static void duer_event_hook(duer_event_t *event)
{
	if (NULL == event) {
		DUER_LOGE("event NULL!!");
		return ;
	}

	switch (event->_event) {
	case DUER_EVENT_STARTED:
		printf("hujie %s:%d DUER_EVENT_STARTED", __func__, __LINE__);
		duerapp_dcs_init();
		b_duer_start = true;
		break;
	case DUER_EVENT_STOPPED:
		b_duer_start = false;
		break;
	}
}

static const char *duerapp_load_profile(const char *profile)
{
	char *data = NULL;
	FILE *fp = NULL;

	do {
		if (NULL == profile) {
			DUER_LOGE("profile path name is NULL!!");
			break;
		}

		fp = fopen(profile, "rb");
		if (NULL == fp) {
			DUER_LOGE("open profile file failed!!");
			break;
		}

		int rs = fseek(fp, 0, SEEK_END);
		if (rs != 0) {
			DUER_LOGE("fseek to file tail failed!!");
			return NULL;
		}

		long size = ftell(fp);
		if (size < 0) {
			DUER_LOGE("ftall failed, file size less than 0!!");
			break;
		}

		rs = fseek(fp, 0, SEEK_SET);
		if (rs != 0) {
			DUER_LOGE("fseek to file head failed!!");
			break;
		}

		data = duer_malloc(size + 1);
		if (NULL == data) {
			DUER_LOGE("duer_malloc data failed!!");
			break;
		}

		rs = fread(data, 1, size, fp);
	} while (0);

	if (NULL != fp) {
		fclose(fp);
	}

	return data;
}

static void duerapp_test_start(const char *profile)
{
	const char *data = duerapp_load_profile(profile);
	if (NULL == data) {
		DUER_LOGE("load profile failed!!");
		return ;
	}
	
	duer_start(data, strlen(data));

	duer_free((void *)data);
}

void duer_dcs_stop_listen_handler(void)
{
	DUER_LOGI("hujie ENTER %s:%d \n", __func__, __LINE__);
	duerapp_record_capture_onoff();
	DUER_LOGI("hujie EXIT %s:%d \n", __func__, __LINE__);
}

void duer_dcs_speak_handler(const char * url)
{
	DUER_LOGI("hujie ENTER %s:%d url:%s\n", __func__, __LINE__, url);
	duerapp_media_speak_player(url);
	DUER_LOGI("hujie EXIT %s:%d url:%s\n", __func__, __LINE__, url);
}

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t * audio_info)
{
	DUER_LOGI("hujie ENTER %s:%d url:%s\n", __func__, __LINE__, audio_info->url);
	duerapp_media_audio_player(audio_info->url);
	DUER_LOGI("hujie EXIT %s:%d url:%s\n", __func__, __LINE__, audio_info->url);
}

void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t * audio_info)
{
	DUER_LOGI("hujie ENTER %s:%d url:%s, offset:%d\n", __func__, __LINE__, audio_info->url, audio_info->offset);
	duerapp_media_audio_resume(audio_info->url, audio_info->offset);
	DUER_LOGI("hujie EXIT %s:%d url:%s, offset:%d\n", __func__, __LINE__, audio_info->url, audio_info->offset);
}

void duer_dcs_audio_pause_handler(void)
{
	DUER_LOGI("hujie ENTER %s:%d \n", __func__, __LINE__);
	duerapp_media_audio_pause();
	DUER_LOGI("hujie EXIT %s:%d \n", __func__, __LINE__);
}

int duer_dcs_audio_get_play_progress(void)
{
	int pos;
	DUER_LOGI("hujie ENTER %s:%d \n", __func__, __LINE__);
	pos = duerapp_media_audio_get_position();
	DUER_LOGI("hujie EXIT %s:%d \n", __func__, __LINE__);
	return pos;
}

duer_status_t duer_dcs_render_card_handler(baidu_json * payload)
{
	printf("hujie %s:%d \n", __func__, __LINE__);
	printf("payload->type:%d\n", payload->type);
	if (payload->valuestring)
		printf("payload->valuestring:%s\n", payload->valuestring);
	printf("payload->valueint:%d\n", payload->valueint);
	printf("payload->valuedouble:%lf\n", payload->valuedouble);
	if (payload->string)
		printf("payload->string:%s\n", payload->string);
}

duer_status_t duer_dcs_input_text_handler(const char * text, const char * type)
{
	printf("hujie %s:%d \n", __func__, __LINE__);
	printf("text:%s\n", text);
	printf("type:%s\n", type);
}


int main()
{
	duerapp_args_t duerapp_args = DEFAULT_ARGS;

	duer_initialize();

	duer_set_event_callback(duer_event_hook);

	duerapp_test_start(duerapp_args.profile);

	duerapp_record_init();

	duerapp_media_init();

	duerapp_event_loop();

	duerapp_media_destory();

	duerapp_record_destory();

    return 0;
}
