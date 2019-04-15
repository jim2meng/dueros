/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_recorder.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Record module function implementation.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef ENABLE_TCP_NODELAY
#include <netinet/tcp.h>
#endif
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <semaphore.h>

#include "duerapp_recorder.h"
#include "duerapp_config.h"
#include "lightduer_voice.h"
#include "lightduer_dcs_router.h"
#include <alsa/asoundlib.h>
#include "snowboy-detect-c-wrapper.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#define SAMPLE_RATE         (16000)
#define FRAMES_INIT         (640*4)
#define CHANNEL 	 	  (2)
#define FRAMES_SIZE  	  ((16/8) *CHANNEL)// bytes / sample * channels
//#define PCM_STREAM_CAPTURE_DEVICE	"hw:2,0"
#define PCM_STREAM_CAPTURE_DEVICE	"default"

//#define RECORD_DATA_TO_FILE

static int s_duer_rec_snd_fd=-1;
static int s_duer_rec_recv_fd=-1;
struct sockaddr_in s_duer_rec_addr;

static duer_rec_state_t s_duer_rec_state = RECORDER_STOP;
static pthread_t s_rec_threadID;
static sem_t s_rec_sem;
static duer_rec_config_t *s_index = NULL;
static bool s_is_suspend = false;
static bool s_is_baidu_rec_start = false;
static pthread_t s_rec_send_threadID;
static char * s_kws_model_filename = NULL;
const char *s_tone_url[3] = {"./resources/60.mp3","./resources/61.mp3","./resources/62.mp3"};
	
extern 	void event_record_start();

int duer_set_kws_model_file(char *filename)
{
	if(filename==NULL){
		return 0;
	}
	
	if(s_kws_model_filename==NULL){
		free(s_kws_model_filename);
		s_kws_model_filename=NULL;
	}
	s_kws_model_filename = strdup(filename);
	
	return 0;
}

typedef struct _pcm_header_t {
    char    pcm_header[4];
    size_t  pcm_length;
    char    format[8];
    int     bit_rate;
    short   pcm;
    short   channel;
    int     sample_rate;
    int     byte_rate;
    short   block_align;
    short   bits_per_sample;
    char    fix_data[4];
    size_t  data_length;
} pcm_header_t;

int g_recorder_channel = 0;

static pcm_header_t s_pcm_header = {
    {'R', 'I', 'F', 'F'},
    (size_t)-1,
    {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '},
    0x10,
    0x01,
    0x01,
#if 1 //def SAMPLE_RATE_16K
    0x3E80,
    0x7D00,
#else //8k
    0x1F40,
    0x3E80,
#endif // SAMPLE_RATE_16K
    0x02,
    0x10,
    {'d', 'a', 't', 'a'},
    (size_t)-1
};


static FILE *s_voice_file=NULL;

int duer_store_voice_start(int name_id)
{
    DUER_LOGD("start");
    if (s_voice_file) {
        fclose(s_voice_file);
        s_voice_file = NULL;
    }

    char _name[64];
    snprintf(_name, sizeof(_name), "./%d.wav", name_id);
    s_voice_file = fopen(_name, "wb");
    if (!s_voice_file) {
        DUER_LOGE("can't open file %s", _name);
        return -1;
    } else {
        DUER_LOGD("begin write to file:%s", _name);
    }
    fwrite(&s_pcm_header, 1, sizeof(s_pcm_header), s_voice_file);
    s_pcm_header.data_length = 0;
    s_pcm_header.pcm_length = sizeof(s_pcm_header) - 8;

    return 0;
}

int duer_store_voice_write(const void *data, uint32_t size)
{
    DUER_LOGD("data");
    if (s_voice_file) {
        fwrite(data, 1, size, s_voice_file);
        s_pcm_header.data_length += size;
    }
    return 0;
}

int duer_store_voice_end()
{
    if (s_voice_file) {
        s_pcm_header.pcm_length += s_pcm_header.data_length;
        fseek(s_voice_file, 0, SEEK_SET);
        fwrite(&s_pcm_header, 1, sizeof(s_pcm_header), s_voice_file);
        fclose(s_voice_file);
        s_voice_file = NULL;
        DUER_LOGD("finish the voice record");
    }
    return 0;
}

int stereo_to_mono(int16_t *in,int ilen,int16_t *out,int outlen)
{
	int i=0;
	
	if(CHANNEL==1){
		memcpy(out,in,ilen<<1);
		return ilen;
	}else if(CHANNEL==2){
		for(i=0;i<ilen>>1;i++){
			 if(duer_app_is_test_mode()!=1||(g_recorder_channel==0)){
			      out[i] = (in[2*i]+in[2*i+1])>>1;
			    //out[i] = in[2*i];
			}else if(g_recorder_channel==1){
				  out[i] = in[2*i];    
			}else if(g_recorder_channel==2){
				   out[i] = in[2*i+1];     
			}
		}
		return ilen>>1;
	}else if(CHANNEL==4){
		for(i=0;i<ilen>>2;i++){
			//out[i] = (in[4*i]+in[4*i+1]+in[4*i+2]+in[4*i+3])>>2;
			out[i] = in[4*i+2];
		}
		return ilen>>2;			
	}
	
	return 0;
}

int  duer_recorder_test_start(int channel)
{
	 g_recorder_channel = channel;   
	if(duer_app_is_test_mode()){
		
		if(g_recorder_channel==1){
			duer_media_tone_play("./resources/left.mp3",20000);
		}else if(g_recorder_channel==2){
			   duer_media_tone_play("./resources/right.mp3",20000); 
		}
		duer_store_voice_end();
		duer_store_voice_start(g_recorder_channel);
	}
	
	return 0;  
}

int  duer_recorder_test_end(void)
{
	char fn[32];
	
	sprintf(fn,"./%d.wav",g_recorder_channel);
	duer_store_voice_end();
        if(duer_app_is_test_mode()){
               duer_media_tone_play(fn,20000); 
	}
	
	return 0;
}

static void recorder_thread()
{
	fd_set fdwrite;
	int value=0;

	const char resource_filename[] = "resources/common.res";
	//const char model_filename[] = "snowboy/resources/models/snowboy.umdl";
	//const char model_filename[] = "resources/models/keywords.pmdl,resources/models/peppapig.pmdl";
	//const char sensitivity_str[] = "0.5,0.5";
	//const char model_filename[] = "resources/models/keywords.pmdl";
	char *model_filename=s_kws_model_filename;
	const char sensitivity_str[] = "0.5";
	float audio_gain = 1.1;
	bool apply_frontend = false;

	if(model_filename==NULL){
		model_filename = "resources/models/keywords.pmdl";
	}
	
	// Initializes Snowboy detector.
	SnowboyDetect* detector = SnowboyDetectConstructor(resource_filename,
	                                                 (const char*)model_filename);
	SnowboyDetectSetSensitivity(detector, sensitivity_str);
	SnowboyDetectSetAudioGain(detector, audio_gain);
	SnowboyDetectApplyFrontend(detector, apply_frontend);

	// Initializes PortAudio. You may use other tools to capture the audio.
	value = SnowboyDetectSampleRate(detector);
	DUER_LOGI("samplerate:%d\n",value);
	
	value = SnowboyDetectNumChannels(detector);
	DUER_LOGI("channels:%d\n",value);
	
	value = SnowboyDetectBitsPerSample(detector);
	DUER_LOGI("bits:%d\n",value);
	
    pthread_detach(pthread_self());
    snd_pcm_hw_params_get_period_size(s_index->params, &(s_index->frames), &(s_index->dir));

    if (s_index->frames < 0) {
        DUER_LOGE("Get period size failed!");
        return;
    }
    DUER_LOGI("frames %d dir %d\n",s_index->frames,s_index->dir);
    s_index->size = s_index->frames * FRAMES_SIZE;
    int16_t *buffer = NULL;
    int16_t *mono_buffer = NULL;
    int mono_data_size = 0;
	
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
	
    buffer = (int16_t *)malloc(s_index->size);
    if (!buffer) {
        DUER_LOGE("malloc buffer failed!\n");
    } else {
        memset(buffer, 0, s_index->size);
    }

    mono_buffer = (int16_t *)malloc(s_index->size);
    if (!mono_buffer) {
        DUER_LOGE("malloc buffer failed!\n");
    } else {
        memset(mono_buffer, 0, s_index->size);
    }
	
    while (1)
    {
        int ret = snd_pcm_readi(s_index->handle, buffer, s_index->frames);

        if (ret == -EPIPE) {
            DUER_LOGE("an overrun occurred!");
            snd_pcm_prepare(s_index->handle);
	    continue;
        } else if (ret < 0) {
            DUER_LOGE("read: %s", snd_strerror(ret));
	    continue;
        } else if (ret != (int)s_index->frames) {
            DUER_LOGE("read %d frames!", ret);
	    continue;
        } else {
            // do nothing
	   //printf("ret=%d\n",ret);
        }

	mono_data_size = stereo_to_mono(buffer,s_index->size>>1,mono_buffer,s_index->size>>1);
	
#if 1		
       int result = SnowboyDetectRunDetection(detector,
                                             mono_buffer, mono_data_size, false);
        if (result > 0) {
            DUER_LOGI("Hotword %d detected!\n", result);
			duer_dcs_dialog_cancel();
			duer_media_tone_play(s_tone_url[rand()%3],5000);
			event_record_start();
			#ifdef RECORD_DATA_TO_FILE
			duer_store_voice_end();
			duer_store_voice_start(time(NULL));
			#else
			if(duer_app_is_test_mode()){
				    duer_store_voice_end();
				    duer_store_voice_start(g_recorder_channel);
			}
			#endif
        }
#endif
		
	if((RECORDER_START == s_duer_rec_state)&&s_is_baidu_rec_start){
	     struct timeval time = {0, 0};
	    FD_ZERO(&fdwrite);
	    FD_SET(s_duer_rec_snd_fd, &fdwrite);
	    		
	    if(select(s_duer_rec_snd_fd + 1, NULL, &fdwrite, NULL, &time)>0){
		 send(s_duer_rec_snd_fd,mono_buffer,mono_data_size<<1,0);
		 #ifdef RECORD_DATA_TO_FILE
		 duer_store_voice_write(mono_buffer,mono_data_size<<1);
		 #else
		 if(duer_app_is_test_mode()){
			DUER_LOGI("test mode\n");
			duer_store_voice_write(mono_buffer,mono_data_size<<1);
		}
		 #endif
             }else{
		 DUER_LOGI("overloap!!!!!!!!!\n");
	     }
	}
    }
    
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    if(mono_buffer){
         free(mono_buffer);
	 mono_buffer=NULL;	
    }
	
    snd_pcm_drain(s_index->handle);
    snd_pcm_close(s_index->handle);
	
    if(s_index) {
        free(s_index);
        s_index = NULL;
    }	
	SnowboyDetectDestructor(detector);	  
	return;
}

static void recorder_data_send_thread()
{
    char *buffer = NULL;
	int size = s_index->size;
	fd_set fdread;
	struct timeval time = {0, 0};
	int recvlen=0;
	
    pthread_detach(pthread_self());

	DUER_LOGI("recorder_data_send_thread start!\n");	
	s_is_baidu_rec_start = false;
	
	while(1)
	{
		FD_ZERO(&fdread);
		FD_SET(s_duer_rec_recv_fd, &fdread);
		if(select(s_duer_rec_recv_fd + 1, &fdread, NULL, NULL, &time)>0){
			recv(s_duer_rec_recv_fd, buffer, size, 0);
		}else{
			break;
		}
	}
	
	DUER_LOGI("flush data end %d!\n",s_duer_rec_state);
	s_is_baidu_rec_start = true;
    duer_voice_start(16000);
	
    buffer = (char *)malloc(size);
    if (!buffer) {
        DUER_LOGE("malloc buffer failed!\n");
    } else {
        memset(buffer, 0, size);
    }
	
    while (RECORDER_START == s_duer_rec_state)
    {
		FD_ZERO(&fdread);
		time.tv_sec = 1;
		time.tv_usec = 0;
		
		FD_SET(s_duer_rec_recv_fd, &fdread);
		if(select(s_duer_rec_recv_fd + 1, &fdread, NULL, NULL, &time)>0){
			recvlen = recv(s_duer_rec_recv_fd, buffer, size, 0);
			if(recvlen>0){
				printf(".&.");
				duer_voice_send(buffer, recvlen);
			}else{
				DUER_LOGE("recv fail!\n");
			}
		}
    }
	
	s_is_baidu_rec_start = false;
    duer_voice_stop();
	
    if(s_is_suspend) {
        duer_voice_terminate();
        s_is_suspend = false;
    }
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
	
    if(sem_post(&s_rec_sem)) {
        DUER_LOGE("sem_post failed.");
    }

	return ;
	
}

static int duer_open_alsa_pcm()
{
    int ret = DUER_OK;
    int result = (snd_pcm_open(&(s_index->handle), PCM_STREAM_CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, 0));
    if (result < 0)
    {
        DUER_LOGE("\n\n****unable to open pcm device: %s*********\n\n", snd_strerror(ret));
        ret = DUER_ERR_FAILED;
    }
    return ret;
}

static int duer_set_pcm_params()
{
    int ret = DUER_OK;
    snd_pcm_hw_params_alloca(&(s_index->params));
    snd_pcm_hw_params_any(s_index->handle, s_index->params);
    snd_pcm_hw_params_set_access(s_index->handle, s_index->params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(s_index->handle, s_index->params,
                                 SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(s_index->handle, s_index->params,
                                   CHANNEL);
    snd_pcm_hw_params_set_rate_near(s_index->handle, s_index->params,
                                    &(s_index->val), &(s_index->dir));
    snd_pcm_hw_params_set_period_size_near(s_index->handle, s_index->params,
                                           &(s_index->frames), &(s_index->dir));

    int result = snd_pcm_hw_params(s_index->handle, s_index->params);
    if (result < 0)    {
        DUER_LOGE("unable to set hw parameters: %s", snd_strerror(ret));
        ret = DUER_ERR_FAILED;
    }
    return ret;
}

int duer_recorder_start()
{
	int ret = 0;

	DUER_LOGI("duer_recorder_start %d!",s_duer_rec_state);
	
    do {
		
		if(sem_wait(&s_rec_sem)) {
			DUER_LOGE("sem_wait failed.");
			break;
		}
		
        if (RECORDER_STOP == s_duer_rec_state) {
			s_duer_rec_state = RECORDER_START;
            ret = pthread_create(&s_rec_send_threadID, NULL, (void *)recorder_data_send_thread, NULL);
            if(ret != 0)
            {
            	s_duer_rec_state = RECORDER_STOP;
                DUER_LOGE("Create recorder pthread error!");
                break;
            } else {
                pthread_setname_np(s_rec_send_threadID, "recorder");
            }
            
        }
        else{
            DUER_LOGI("Recorder Start failed! state:%d", s_duer_rec_state);
        }
		
        return DUER_OK;
		
    } while(0);
	
    return DUER_ERR_FAILED;
}

int duer_recorder_stop()
{
    int ret = DUER_OK;
	
	DUER_LOGI("duer_recorder_stop! state:%d", s_duer_rec_state);
	
    if (RECORDER_START == s_duer_rec_state) {
        s_duer_rec_state = RECORDER_STOP;
		s_is_baidu_rec_start = false;
    } else {
        ret = DUER_ERR_FAILED;
        DUER_LOGI("Recorder Stop failed! state:%d", s_duer_rec_state);
    }
	
    return ret;
}

int duer_recorder_suspend()
{
    int ret = duer_recorder_stop();
    if (DUER_OK == ret) {
        s_is_suspend = true;
    } else {
        DUER_LOGI("Recorder Stop failed! state:%d", s_duer_rec_state);
    }
    return ret;
}

duer_rec_state_t duer_get_recorder_state()
{
    return s_duer_rec_state;
}

int duer_hotwords_detect_start(char *model_filename)
{
	int ret=0;

	duer_set_kws_model_file(model_filename);
    s_duer_rec_addr.sin_family = AF_INET;
    s_duer_rec_addr.sin_port = htons(19290);
	s_duer_rec_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
    if(sem_init(&s_rec_sem, 0, 1)) {
        DUER_LOGE("Init s_rec_sem failed.");
        return -1;
    }
	
    s_index = (duer_rec_config_t *)malloc(sizeof(duer_rec_config_t));
    if (!s_index) {
		DUER_LOGE("malloc fail");
        return -1;
    }
	
    memset(s_index, 0, sizeof(duer_rec_config_t));
    s_index->frames = FRAMES_INIT;
    s_index->val = SAMPLE_RATE; // pcm sample rate
    
    do{
		s_duer_rec_snd_fd =  socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
		if(s_duer_rec_snd_fd <0){
			DUER_LOGE("open send socket failed");
			break;
		}
		
		ret = connect(s_duer_rec_snd_fd, (struct sockaddr *)&s_duer_rec_addr, sizeof(s_duer_rec_addr));
		if(ret!=0){
			DUER_LOGE("socket connect failed");
			break;
		}
		
		s_duer_rec_recv_fd =  socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
		if(s_duer_rec_recv_fd <0){
			DUER_LOGE("open recv socket failed");
			break;
		}

		ret = bind(s_duer_rec_recv_fd, (struct sockaddr *) &s_duer_rec_addr, sizeof(s_duer_rec_addr));
		if(ret<0){
			DUER_LOGE("socket bind failed");
			break;
		}
		
	    ret = duer_open_alsa_pcm();
	    if (ret != DUER_OK) {
	        DUER_LOGE("open pcm failed");
	        break;
	    }
		
	    ret = duer_set_pcm_params();
	    if (ret != DUER_OK) {
	        DUER_LOGE("open pcm failed");
	        break;
	    }

	    ret = pthread_create(&s_rec_threadID, NULL, (void *)recorder_thread, NULL);
	    if(ret != 0){
	        DUER_LOGE("Create recorder pthread error!");
	        break;
	    } else {
	        pthread_setname_np(s_rec_threadID, "recorder");
	    }
    }while(0);

	if(ret!=0){
		if(s_index) {
        	free(s_index);
        	s_index = NULL;
    	}
		if(s_duer_rec_recv_fd>=0){
			close(s_duer_rec_recv_fd);
			s_duer_rec_recv_fd = -1;
		}
		
		if(s_duer_rec_snd_fd>=0){
			close(s_duer_rec_snd_fd);
			s_duer_rec_snd_fd = -1;
		}		
	}
	
    return ret;
	
}

