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
#include "windows.h"//��Ҫ����Sleep����

extern "C" {
#include "libavutil\time.h"
#include "libswresample\swresample.h"
}

#define NUMBUFFERS (4)//�󶨵�Դ�ϵ�buffer����

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

//���ȳ�ʼ���豸�����
static int init_audio_device()
{
    //͵��ֱ�Ӵ�Ĭ���豸
    device = alcOpenDevice(NULL);
    if (!device)
        return -1;

    //���豸�ϴ���������
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

//PCM��ʽת������Ҫ����ΪOpenAL����ֱ��ʹ��ƽ���ʽ(planar)
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

//��ȡpcm��ʽ
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

    //����NUMBUFFERS��buffer
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
            //planar��ʽ����Ҫ�Լ�ת����packed
            buff_size = pcm_fmt_tran(g_player_ctx->audio_codec_ctx, audio_frame);
            alBufferData(uiBuffers[i], pcm_fmt, flush_buff, buff_size, g_player_ctx->audio_codec_ctx->sample_rate);
            memset(flush_buff, 0, sizeof(flush_buff));
        } else {
            //����buffer����
            alBufferData(uiBuffers[i], pcm_fmt, *audio_frame->data, 
                buff_size, g_player_ctx->audio_codec_ctx->sample_rate);
        }

        //��PCM��ʽ����Ƶbuffer�ŵ�Դ�Ķ���
        alGetError();
        alSourceQueueBuffers(uiSource, 1, uiBuffers + i);
        buffer_meta[i].buffer = uiBuffers[i];
        buffer_meta[i].len = buff_size;
      //  write_pcm_data(audio_frame->data[0], buff_size);

        av_free(audio_frame);
    }

    //��ʼ����Դ
    alSourcePlay(uiSource);

    //��¼��ʼ����ʱ��
    //pthread_mutex_lock(&g_player_ctx->audio_time_mutex);
    g_player_ctx->audio_clock = 0;
    ref_time = GetTickCount();
    //pthread_mutex_unlock(&g_player_ctx->audio_time_mutex);

    while (true) {

        //Sleep(20);

        //��ѯ�Ѿ����ŵ�buffer������
        alGetSourcei(uiSource, AL_BUFFERS_PROCESSED, &bufferProcessed);
        while (bufferProcessed) {

            ALuint uiUnqueue;
            alSourceUnqueueBuffers(uiSource, 1, &uiUnqueue); //ȡ��1���������buffer��ID(uiUnqueue)

            buff_size = get_buffer_len(buffer_meta, NUMBUFFERS, uiUnqueue);
            //printf("buff_id = %d, buff_size = %d\n", uiUnqueue, buff_size);

            //������Ƶʱ��
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
                //planar��ʽ����Ҫ�Լ�ת����packed
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
  
        ////��ѯ״̬
        //alGetSourcei(uiSource, AL_SOURCE_STATE, &iState);
        //if (iState != AL_PLAYING) {
        //    ALint iQueuedBuffers = 0;
        //    alGetSourcei(uiSource, AL_BUFFERS_QUEUED, &iQueuedBuffers);
        //    
        //    if (iQueuedBuffers)//�����л���buffer������֮��
        //        alSourcePlay(uiSource);
        //    else
        //        break;
        //}
    }

    //�˳�ǰ����
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
