

#include "duerapp_record.h"
#include "duerapp_media.h"

#include "lightduer_types.h"

#include "lightduer_dcs.h"
#include "lightduer_voice.h"

#define DEFAULT_FORMAT		SND_PCM_FORMAT_S16_LE
#define DEFAULT_RATE		16000 // 16k
#define DEFAULT_CHANNEL		1 // 左声道
#define DEFAULT_FRAMES		16 // 16帧/周期

static duerapp_record_config_t *s_record_config = NULL;

static int duerapp_record_pcm_open(duerapp_record_config_t *config, snd_pcm_stream_t mode)
{
	/* mode: SND_PCM_STREAM_CAPTURE 录音 */
	int ret = snd_pcm_open(&config->handle, "default", mode, 0);
	if (ret < 0) {
		DUER_LOGE("snd_pcm_open failed: %s", snd_strerror(ret));
		return -1;
	}

	return 0;
}

static int duerapp_record_pcm_close(duerapp_record_config_t *config)
{
	snd_pcm_close(config->handle);
	config->handle = NULL;

	return 0;
}

static int duerapp_record_pcm_setparams(duerapp_record_config_t *config)
{
	int ret;

	if (NULL == config) {
		DUER_LOGE("params NULL!!");
		return -1;
	}

	/* 分配硬件参数对象 */
	snd_pcm_hw_params_alloca(&config->params);

	/* 填充默认值 */
	snd_pcm_hw_params_any(config->handle, config->params);

	/* 设置所需的硬件参数 */

	/* 设置访问类型：交错模式
	 * SND_PCM_ACCESS_RW_INTERLEAVED 交错模式
	 * SND_PCM_ACCESS_RW_NONINTERLEAVED 非交错模式 
	 */
	snd_pcm_hw_params_set_access(config->handle, config->params, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* 设置数据格式：
	 * SND_PCM_FORMAT_S16_LE 有符号16bit Little Endian
	 * S:有符号    U:无符号
	 * 8:bit  16:16bit  32:32bit
	 * LE:Little Endian  BE:Big Endian
	 */
	snd_pcm_hw_params_set_format(config->handle, config->params, DEFAULT_FORMAT);

	/* 设置声道：
	 * 1 单声道  2 立体声
	 */
	snd_pcm_hw_params_set_channels(config->handle, config->params, DEFAULT_CHANNEL);

	/* 设置采样率：
	 * 例如4100bit/s 采样率(CD质量)
	 */
	snd_pcm_hw_params_set_rate_near(config->handle, config->params,
				&config->rate, NULL);

	/* 设置周期大小为32帧 */
	snd_pcm_hw_params_set_period_size_near(config->handle, config->params,
				&config->frames, NULL);

	/* 将参数设置到驱动程序 */
	ret = snd_pcm_hw_params(config->handle, config->params);
	if (ret < 0) {
		DUER_LOGE("snd_pcm_hw_params failed!!");
		return -2;
	}

	return 0;
}

static void duerapp_record_pcm_capture(duerapp_record_config_t *config)
{
	char *data = NULL;
	long size;

	snd_pcm_hw_params_get_period_size(config->params, &config->frames, NULL);
	size = config->frames * DEFAULT_CHANNEL * 2;
	data = duer_malloc(size);
	if (NULL == data) {
		DUER_LOGE("duer_malloc data failed!!");
		return ;
	}

	FILE *fp = fopen("/home/hujie/listen.mp3", "wb");
	duer_dcs_on_listen_started();
	duer_voice_start(config->rate);
	while (config->state == RECORD_CAPTURE) {
		memset(data, 0, size + 1);
		int rc = snd_pcm_readi(config->handle, data, config->frames);
		if (-EPIPE == rc) {
			DUER_LOGE("overrun occurred");
			snd_pcm_prepare(config->handle);
			continue;
		} else if (rc < 0) {
			DUER_LOGE("snd_pcm_readi failed: %s", snd_strerror(rc));
			continue;
		} else if (rc != (int)config->frames) {
			DUER_LOGE("short read, read %d frames", rc);
			continue;
		}
		// 发送录音的数据
		duer_voice_send(data, size);
		fwrite(data, size, 1, fp);
	}
	duer_voice_stop();
	duer_dcs_on_listen_stopped();
	duer_free(data);
	fclose(fp);

	snd_pcm_drain(config->handle);
}

static void duerapp_record_wait(duerapp_record_config_t *config)
{
	pthread_mutex_lock(&config->mutex);
	pthread_cond_wait(&config->cond, &config->mutex);
	pthread_mutex_unlock(&config->mutex);
}

static void duerapp_record_awake(duerapp_record_config_t *config)
{
	pthread_mutex_lock(&config->mutex);
	pthread_cond_signal(&config->cond);
	pthread_mutex_unlock(&config->mutex);
}

static void duerapp_record_turn_state(duerapp_record_config_t *config, int state)
{
	if (config->state == RECORD_UNCAPTURE) {
		config->state = state;
		duerapp_record_awake(config);
	} else {
		config->state = state;
	}
}

static void *duerapp_record_running(void *arg)
{
	bool loop = true;
	duerapp_record_config_t *config = (duerapp_record_config_t *)arg;

	

	while (loop) {
		switch (config->state) {
		case RECORD_CAPTURE:
			if (0 != duerapp_record_pcm_open(config, SND_PCM_STREAM_CAPTURE)) {
				return NULL;
			}

			if (0 != duerapp_record_pcm_setparams(config)) {
				return NULL;
			}
			duerapp_record_pcm_capture(config);
			duerapp_record_pcm_close(config);
			break;
		case RECORD_UNCAPTURE:
			duerapp_record_wait(config);
			break;
		case RECORD_QUIT:
			loop = false;
			break;
		}
	}

	return NULL;
}

void duerapp_record_init(void)
{
	s_record_config = (duerapp_record_config_t *)duer_malloc(sizeof(duerapp_record_config_t));
	if (NULL == s_record_config) {
		DUER_LOGE("duer_malloc failed!!");
		exit(EXIT_FAILURE);
	}
	memset(s_record_config, 0, sizeof(duerapp_record_config_t));
	s_record_config->state = RECORD_UNCAPTURE;
	s_record_config->rate = DEFAULT_RATE;
	s_record_config->frames = DEFAULT_FRAMES;

	pthread_mutex_init(&s_record_config->mutex, NULL);
	pthread_cond_init(&s_record_config->cond, NULL);

	int ret = pthread_create(&s_record_config->thread_id, NULL, duerapp_record_running, (void *)s_record_config);
	if (ret != 0) {
		DUER_LOGE("create duerapp_record_config failed!!");
		duerapp_record_destory();
		exit(EXIT_FAILURE);
	}
}

void duerapp_record_destory(void)
{
	// 结束媒体
	duerapp_record_turn_state(s_record_config, RECORD_QUIT);
	if (s_record_config->thread_id > 0) {
		pthread_join(s_record_config->thread_id, NULL);
		s_record_config->thread_id = 0;
	}

	pthread_mutex_destroy(&s_record_config->mutex);
	pthread_cond_destroy(&s_record_config->cond);

	if (s_record_config){
		duer_free(s_record_config);
	}
}

void duerapp_record_capture_onoff(void)
{
	if (s_record_config->state == RECORD_UNCAPTURE) {
		// 暂停媒体
		duerapp_media_speak_stop();
		//duerapp_media_audio_pause();
		duerapp_record_turn_state(s_record_config, RECORD_CAPTURE);
	} else if (s_record_config->state == RECORD_CAPTURE) {
		duerapp_record_turn_state(s_record_config, RECORD_UNCAPTURE);
	}
}
