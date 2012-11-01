/** 
* @file         audio_render.cpp 
* @Synopsis     audio render into OpenAL device 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include "file_util.h"
#include "player_common.h"
#include "audio_render.h"
#include "al.h"
#include "alc.h"
#include "windows.h"//需要调用Sleep函数

extern "C" {
#include "libavutil\time.h"
#include "libswresample\swresample.h"
}

#define NUMBUFFERS (4)//绑定到源上的buffer数量

static ALCdevice *device;
static ALCcontext *cc;
DWORD ref_time;
DECLARE_ALIGNED(16,uint8_t,flush_buff)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];

struct audio_buffer_meta{
    ALuint buffer;
    int len;
};

void init_buffer_meta(struct audio_buffer_meta meta[], int len)
{
    memset(meta, 0, len * sizeof(struct audio_buffer_meta));
}

int get_buffer_len(struct audio_buffer_meta meta[], int len, ALuint buffer)
{
    int i = 0;
    for (i; i < len; i++) {
        if (buffer == meta[i].buffer) {
            return meta[i].len;
        }
    }

    return 0;
}

void set_buffer_len(struct audio_buffer_meta meta[], int len, ALuint buffer, int buffer_len)
{
    int i = 0;
    for (i; i < len; i++) {
        if (buffer == meta[i].buffer) {
            meta[i].len = buffer_len;
        }
    }

}

//首先初始化设备神马的
static int init_audio_device()
{
    //偷懒直接打开默认设备
    device = alcOpenDevice(NULL);
    if (!device)
        return -1;

    //在设备上创建上下文
    cc = alcCreateContext(device, NULL);
    alcMakeContextCurrent(cc);

    return 0;
}

static void destroy_audio_device()
{
    alcMakeContextCurrent(NULL);
    alcDestroyContext(cc);
    alcCloseDevice(device);
}

//PCM格式转换，主要是因为OpenAL不能直接使用平面格式(planar)
static int pcm_fmt_tran(AVCodecContext *codec_ctx, AVFrame *frame)
{
    static struct SwrContext *swr;
    static int out_count;
    static uint8_t *out[1];
    static int sample_size;
    if (!swr) {
        swr = swr_alloc();
        swr_alloc_set_opts(swr, 3, AV_SAMPLE_FMT_S16, codec_ctx->sample_rate, 3, AV_SAMPLE_FMT_S16P,
            codec_ctx->sample_rate, 0, NULL);
        
        if (swr_init(swr) < 0)
        return -1;

        out_count = sizeof(flush_buff) / 2 / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        out[0] = flush_buff;
        sample_size = (codec_ctx->channels) * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    }

    if (!flush_buff)
        return -1;
    
    const uint8_t **in = (const uint8_t**)frame->extended_data;

    int sample_count = swr_convert(swr, out, out_count, in, frame->nb_samples);

    if (sample_count < 0)
        return -1;

    return sample_count * sample_size;
}

//获取pcm格式
static inline ALenum get_pcm_format(AVCodecContext *codec_ctx)
{
    if (codec_ctx->sample_fmt == AV_SAMPLE_FMT_U8 || codec_ctx->sample_fmt == AV_SAMPLE_FMT_U8P)
        if (codec_ctx->channels == 1)
            return AL_FORMAT_MONO8;
        else if (codec_ctx->channels == 2)
            return AL_FORMAT_STEREO8;

    if (codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16 || codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16P)
        if (codec_ctx->channels == 1)
            return AL_FORMAT_MONO16;
        else if (codec_ctx->channels == 2)
            return AL_FORMAT_STEREO16;

    printf("Open AL can't render this PCM format!\n");
    return -1;
}

void *audio_player(void*)
{
    //open_pcm_file("test.wav");

    if (init_audio_device() < 0)
        return NULL;

    ALint bufferProcessed = 0;
    ALint iState = 0;
    ALuint uiBuffers[NUMBUFFERS];
    struct audio_buffer_meta buffer_meta[NUMBUFFERS];
    ALuint uiSource;
    static ALenum pcm_fmt = 0;

    alGenSources(1, &uiSource);

    alGenBuffers(NUMBUFFERS, uiBuffers);
    int ii = 0;
    for (ii; ii < NUMBUFFERS; ii++) {
        printf("uiBuffers[] : %d\n", uiBuffers[ii]);
    }

    init_buffer_meta(buffer_meta, NUMBUFFERS);
    AVFrame *audio_frame = NULL;
    int buff_size = 0;  

    //放入NUMBUFFERS个buffer
    int i;
    for (i = 0; i < NUMBUFFERS; i++) {

        audio_frame = (AVFrame*)pop_from_queue(&audio_sam_queue);
        buff_size = av_samples_get_buffer_size(NULL, g_player_ctx->audio_codec_ctx->channels, 
            audio_frame->nb_samples, g_player_ctx->audio_codec_ctx->sample_fmt, 1);

        if (!pcm_fmt)
            pcm_fmt = get_pcm_format(g_player_ctx->audio_codec_ctx);
        if (pcm_fmt == -1)
            return NULL;

        alGetError();
        if (av_sample_fmt_is_planar(g_player_ctx->audio_codec_ctx->sample_fmt)) {
            //planar格式，需要自己转换成packed
            buff_size = pcm_fmt_tran(g_player_ctx->audio_codec_ctx, audio_frame);
            alBufferData(uiBuffers[i], pcm_fmt, flush_buff, buff_size, g_player_ctx->audio_codec_ctx->sample_rate);
            memset(flush_buff, 0, sizeof(flush_buff));
        } else {
            //生成buffer数据
            alBufferData(uiBuffers[i], pcm_fmt, *audio_frame->data, 
                buff_size, g_player_ctx->audio_codec_ctx->sample_rate);
        }

        //将PCM格式的音频buffer放到源的队列
        alGetError();
        alSourceQueueBuffers(uiSource, 1, uiBuffers + i);
        buffer_meta[i].buffer = uiBuffers[i];
        buffer_meta[i].len = buff_size;
      //  write_pcm_data(audio_frame->data[0], buff_size);

        av_free(audio_frame);
    }

    //开始播放源
    alSourcePlay(uiSource);

    //记录开始播放时间
    //pthread_mutex_lock(&g_player_ctx->audio_time_mutex);
    g_player_ctx->audio_clock = 0;
    ref_time = GetTickCount();
    //pthread_mutex_unlock(&g_player_ctx->audio_time_mutex);

    while (true) {

        //Sleep(20);

        //查询已经播放的buffer的数量
        alGetSourcei(uiSource, AL_BUFFERS_PROCESSED, &bufferProcessed);
        while (bufferProcessed) {

            ALuint uiUnqueue;
            alSourceUnqueueBuffers(uiSource, 1, &uiUnqueue); //取出1个播放完的buffer的ID(uiUnqueue)

            buff_size = get_buffer_len(buffer_meta, NUMBUFFERS, uiUnqueue);
            //printf("buff_id = %d, buff_size = %d\n", uiUnqueue, buff_size);

            //修正音频时钟
            //pthread_mutex_lock(&g_player_ctx->audio_time_mutex);
            g_player_ctx->audio_clock += (double)buff_size / (double)(g_player_ctx->audio_bytes_per_sec);
            //printf("%f\n", g_player_ctx->audio_clock);
            ref_time = GetTickCount();
            //pthread_mutex_unlock(&g_player_ctx->audio_time_mutex);


            audio_frame = (AVFrame*)pop_from_queue(&audio_sam_queue);
            buff_size = av_samples_get_buffer_size(NULL, g_player_ctx->audio_codec_ctx->channels, 
                audio_frame->nb_samples, g_player_ctx->audio_codec_ctx->sample_fmt, 1);

            alGetError();
            if (av_sample_fmt_is_planar(g_player_ctx->audio_codec_ctx->sample_fmt)) {
                //planar格式，需要自己转换成packed
                buff_size = pcm_fmt_tran(g_player_ctx->audio_codec_ctx, audio_frame);
                alBufferData(uiBuffers[i], pcm_fmt, flush_buff, buff_size, g_player_ctx->audio_codec_ctx->sample_rate);
                memset(flush_buff, 0, sizeof(flush_buff));
            } else {
                alBufferData(uiUnqueue, pcm_fmt, *audio_frame->data, 
                    buff_size, g_player_ctx->audio_codec_ctx->sample_rate);
            }

            alGetError();
            alSourceQueueBuffers(uiSource, 1, &uiUnqueue);
            set_buffer_len(buffer_meta, NUMBUFFERS, uiUnqueue, buff_size);
            bufferProcessed--;

            //write_pcm_data(audio_frame->data[0], buff_size);

            av_free(audio_frame);
        }
  
        ////查询状态
        //alGetSourcei(uiSource, AL_SOURCE_STATE, &iState);
        //if (iState != AL_PLAYING) {
        //    ALint iQueuedBuffers = 0;
        //    alGetSourcei(uiSource, AL_BUFFERS_QUEUED, &iQueuedBuffers);
        //    
        //    if (iQueuedBuffers)//队列中还有buffer，播放之。
        //        alSourcePlay(uiSource);
        //    else
        //        break;
        //}
    }

    //退出前清理
    alDeleteBuffers(NUMBUFFERS, uiBuffers);
    alDeleteSources(1, &uiSource);
    destroy_audio_device();
    return NULL;
}

double get_current_audio_clock()
{
    double audio_clock;
    //pthread_mutex_lock(&g_player_ctx->audio_time_mutex);
    audio_clock = g_player_ctx->audio_clock + (double)(GetTickCount() - ref_time) / 1000;
    //printf("audio clock %f\n", audio_clock);
    //pthread_mutex_unlock(&g_player_ctx->audio_time_mutex);
    return audio_clock;
}
