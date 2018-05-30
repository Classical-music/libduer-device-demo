
#include "duerapp_media.h"

#include "lightduer_dcs.h"

duerapp_media_config_t *s_speak_config = NULL;
duerapp_media_config_t *s_audio_config = NULL;

static void duerapp_media_gst_destory(duerapp_media_config_t * config);
static void duerapp_media_config_destory(duerapp_media_config_t * config);

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *)data;

	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS:
		DUER_LOGI("hujie %s:%d \n", __func__, __LINE__);
		g_print("End of stream\n");
		g_main_loop_quit(loop);
		break;

	case GST_MESSAGE_ERROR: {
		DUER_LOGI("hujie %s:%d \n", __func__, __LINE__);
		gchar *debug;
		GError *error;

		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);

		g_printerr("Error: %s\n", error->message);
		g_error_free(error);

		g_main_loop_quit(loop);
		break;
	}
	default:
		break;
	}
}

static void duerapp_media_gst_init(duerapp_media_config_t *config)
{
	GstBus *bus = NULL;

	if (!gst_is_initialized())
		gst_init(NULL, NULL);

	config->loop = g_main_loop_new(NULL, FALSE);
	if (NULL == config->loop) {
		DUER_LOGE("g_main_loop_new failed!!");
		exit(EXIT_FAILURE);
	}

	config->pipeline = gst_element_factory_make("playbin", config->name);
	if (NULL == config->pipeline) {
		DUER_LOGE("factory_make pipeline failed!!");
		duerapp_media_gst_destory(config);
		exit(EXIT_FAILURE);
	}

	bus = gst_pipeline_get_bus(GST_PIPELINE(config->pipeline));
	if (NULL == bus) {
		DUER_LOGE("pipeline get bus failed!!");
		duerapp_media_gst_destory(config);
		exit(EXIT_FAILURE);
	}
	config->bus_watch_id = gst_bus_add_watch(bus, bus_call, config->loop);
	gst_object_unref(bus);
	gst_element_set_state(config->pipeline, GST_STATE_NULL);
}

static void duerapp_media_gst_setparam(duerapp_media_config_t *config)
{
	if (config->url)
		g_object_set(G_OBJECT(config->pipeline), "uri", config->url, NULL);
	if (config->mute)
		g_object_set(G_OBJECT(config->pipeline), "volume", 0, NULL);
	else
		g_object_set(G_OBJECT(config->pipeline), "volume", config->volume, NULL);
}

static void duerapp_media_gst_run(duerapp_media_config_t *config)
{
	gst_element_set_state(config->pipeline, GST_STATE_PLAYING);
	g_main_loop_run(config->loop);
}

static void duerapp_media_gst_destory(duerapp_media_config_t *config)
{
	if (config->pipeline) {
		gst_element_set_state(config->pipeline, GST_STATE_NULL);

		gst_object_unref(GST_OBJECT(config->pipeline));
		config->pipeline = NULL;
	}

	if (config->bus_watch_id) {
		g_source_remove(config->bus_watch_id);
		config->bus_watch_id = 0;
	}

	if (config->loop) {
		g_main_loop_unref(config->loop);
		config->loop = NULL;
	}
}

static void duerapp_media_pthread_wait(duerapp_media_config_t *config)
{
	pthread_mutex_lock(&config->mutex);
	pthread_cond_wait(&config->cond, &config->mutex);
	pthread_mutex_unlock(&config->mutex);
}

static void duerapp_media_pthread_awake(duerapp_media_config_t *config)
{
	pthread_mutex_lock(&config->mutex);
	pthread_cond_signal(&config->cond);
	pthread_mutex_unlock(&config->mutex);
}

static void duerapp_media_turn_state(duerapp_media_config_t *config, int state)
{
	DUER_LOGI("hujie %s:%d name:%s oldstate:%d newstate:%d\n", __func__, __LINE__, config->name, config->state, state);
	if (config->state == state
	|| (config->state == MEDIA_STOP && state == MEDIA_PAUSE))
		return ;
	config->laststate = config->state;
	config->state = state;
	switch (config->laststate) {
	case MEDIA_PLAY:
		g_main_loop_quit(config->loop);
		break;
	case MEDIA_PAUSE:
	case MEDIA_STOP:
		duerapp_media_pthread_awake(config);
		break;
	default:
		break;
	}
}

static void *duerapp_media_running(void *arg)
{
	bool loop = true;
	duerapp_media_config_t *config = (duerapp_media_config_t *)arg;

	while (loop) {
		switch (config->state) {
		case MEDIA_PLAY:
			if (config->laststate == MEDIA_STOP) {
				duerapp_media_gst_init(config);
			}
			duerapp_media_gst_setparam(config);
			duerapp_media_gst_run(config);
			if (config->state  == MEDIA_PLAY) {
				config->laststate = config->state;
				config->state = MEDIA_STOP;
			}
			break;
		case MEDIA_PAUSE:
			gst_element_set_state(config->pipeline, GST_STATE_PAUSED);
			duerapp_media_pthread_wait(config);
			break;
		case MEDIA_STOP:
			if (config->laststate != MEDIA_STOP) {
				duerapp_media_gst_destory(config);
			}
			if (config->laststate == MEDIA_PLAY && config->duerapp_media_player_finished) {
				config->duerapp_media_player_finished();
			}
			duerapp_media_pthread_wait(config);
			break;
		case MEDIA_QUIT:
			if (config->laststate != MEDIA_STOP)
				duerapp_media_gst_destory(config);
			loop = false;
			break;
		}
	}
}

static duerapp_media_config_t *duerapp_media_config_init(void)
{
	duerapp_media_config_t *config = duer_malloc(sizeof(duerapp_media_config_t));
	if (NULL == config) {
		DUER_LOGE("duer_malloc failed!!");
		return NULL;
	}
	memset(config, 0, sizeof(duerapp_media_config_t));
	config->state = MEDIA_STOP;
	config->laststate = MEDIA_STOP;
	config->volume = 0.5;

	pthread_mutex_init(&config->mutex, NULL);
	pthread_cond_init(&config->cond, NULL);

	int ret = pthread_create(&config->thread_id, NULL, duerapp_media_running, (void *)config);
	if (0 != ret) {
		DUER_LOGE("create media thread failed!!");
		duerapp_media_config_destory(config);
		exit(EXIT_FAILURE);
	}

	return config;
}

static void duerapp_media_config_destory(duerapp_media_config_t *config)
{
	if (!config)
		return ;

	duerapp_media_turn_state(config, MEDIA_QUIT);
	if (config->thread_id) {
		DUER_LOGI("hujie %s:%d %s\n", __func__, __LINE__, config->name);
		pthread_join(config->thread_id, NULL);
		DUER_LOGI("hujie %s:%d %s\n", __func__, __LINE__, config->name);
	}

	pthread_cond_destroy(&config->cond);
	pthread_mutex_destroy(&config->mutex);

	duer_free(config);
}

static void duerapp_media_speak_player_finished(void)
{
	DUER_LOGI("hujie %s:%d \n", __func__, __LINE__);
	if (s_audio_config->state == MEDIA_PAUSE) {
		//duerapp_media_turn_state(s_audio_config, MEDIA_PLAY);
	}
	DUER_LOGI("hujie %s:%d \n", __func__, __LINE__);
	duer_dcs_speech_on_finished();
}

static void duerapp_media_audio_player_finished(void)
{
	duer_dcs_audio_on_finished();
}

void duerapp_media_init(void)
{
	s_speak_config = duerapp_media_config_init();
	if (NULL == s_speak_config) {
		DUER_LOGE("speak_config init failed!!");
		exit(EXIT_FAILURE);
	}
	s_speak_config->name = "speak";
	s_speak_config->duerapp_media_player_finished = duerapp_media_speak_player_finished;
	s_audio_config = duerapp_media_config_init();
	if (NULL == s_audio_config) {
		DUER_LOGE("audio_config init failed!!");
		duerapp_media_config_destory(s_speak_config);
	}
	s_audio_config->name = "audio";
	s_audio_config->duerapp_media_player_finished = duerapp_media_audio_player_finished;
}

void duerapp_media_destory(void)
{
	duerapp_media_config_destory(s_audio_config);
	s_audio_config = NULL;
	duerapp_media_config_destory(s_speak_config);
	s_speak_config = NULL;
}

void duerapp_media_speak_player(const char *url)
{
	// 播放新 speak
	s_speak_config->url = url;
	DUER_LOGI("hujie %s:%d speak state:%d\n", __func__, __LINE__, s_speak_config->state);
	duerapp_media_turn_state(s_speak_config, MEDIA_PLAY);
}

void duerapp_media_speak_stop(void)
{
	s_speak_config->url = NULL;
	duerapp_media_turn_state(s_speak_config, MEDIA_STOP);
}

void duerapp_media_audio_player(const char *url)
{
	// 停止旧媒体
	duerapp_media_turn_state(s_speak_config, MEDIA_STOP);
	duerapp_media_turn_state(s_audio_config, MEDIA_STOP);

	// 播放新 audio
	s_audio_config->url = url;
	//duerapp_media_gst_setparam(s_audio_config);
	duerapp_media_turn_state(s_audio_config, MEDIA_PLAY);
}

void duerapp_media_audio_pause(void)
{
	if (s_audio_config->state == MEDIA_PLAY) {
		duerapp_media_turn_state(s_audio_config, MEDIA_PAUSE);
	}
}

void duerapp_media_audio_resume(const char *url, int offset)
{
	if (s_audio_config->state == MEDIA_PAUSE && s_audio_config->url == url) {
		duerapp_media_turn_state(s_audio_config, MEDIA_PLAY);
	}
}

void duerapp_media_audio_stop(void)
{
	s_audio_config->url = NULL;
	duerapp_media_turn_state(s_audio_config, MEDIA_STOP);
}

int duerapp_media_audio_get_position(void)
{
	gint64 pos, len;

	if (gst_element_query_position (s_audio_config->pipeline, GST_FORMAT_TIME, &pos)
	&& gst_element_query_duration (s_audio_config->pipeline, GST_FORMAT_TIME, &len)) {
		g_print ("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
		GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
	}

	return pos * 100 * len;
}

void duerapp_media_audio_onoff(void)
{
	DUER_LOGI("hujie %s:%d state:%d\n", __func__, __LINE__, s_audio_config->state);
	if (s_audio_config->state == MEDIA_PLAY) {
		duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
	} else/* if (s_audio_config->state == MEDIA_PAUSE)*/ {
		duer_dcs_send_play_control_cmd(DCS_PLAY_CMD);
	}
}

void duerapp_media_audio_last(void)
{
	duer_dcs_send_play_control_cmd(DCS_PREVIOUS_CMD);
}

void duerapp_media_audio_next(void)
{
	duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
}
