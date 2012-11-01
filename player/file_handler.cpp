/** 
* @file         file_handler.cpp 
* @Synopsis     open decoder for the file, Demux the file. 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include "player_common.h"
#include "file_handler.h"

int get_buffer_with_pts(struct AVCodecContext *ctx, AVFrame *frame)
{
    int ret = avcodec_default_get_buffer(ctx, frame);
    uint64_t *pts = (uint64_t*)av_malloc(sizeof(uint64_t));
    *pts = AV_NOPTS_VALUE;
    frame->opaque = pts;
    return ret;
}

void release_buffer_with_pts(struct AVCodecContext *ctx, AVFrame *frame)
{
    if (frame)
    {
        av_freep(&frame->opaque);
    }
    avcodec_default_release_buffer(ctx, frame);
}

int initDecoderWithFile(const char* filename)
{
    if (!filename)
        return -1;

    avcodec_register_all();
    av_register_all();

    if (!g_player_ctx) {
        g_player_ctx = (struct player_context*)malloc(sizeof(struct player_context));
        memset(g_player_ctx, 0, sizeof(struct player_context));
        pthread_mutex_init(&g_player_ctx->audio_time_mutex, NULL);
    }

    if (avformat_open_input(&g_player_ctx->fmt_ctx, filename, NULL, NULL) != 0) {
        //�򿪶�ý���ļ�
        printf("open file [%s] error!\n", filename);
        return -1;
    }

    if (avformat_find_stream_info(g_player_ctx->fmt_ctx, NULL) < 0) {
        //���Ҷ�ý���ļ�������Ϣ
        avformat_close_input(&g_player_ctx->fmt_ctx);
        printf("get file info failed!\n");
        return -1;
    }

    av_dump_format(g_player_ctx->fmt_ctx, 0, filename, false);//dump���ļ���ʽ

    g_player_ctx->video_id = -1;//Ϊֻ����Ƶ���ļ���ǳ���
    unsigned int i;
    for (i = 0; i < g_player_ctx->fmt_ctx->nb_streams; ++i) {
        if (g_player_ctx->fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            g_player_ctx->video_id = i;//��¼��Ƶ���ı��

        if (g_player_ctx->fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            g_player_ctx->audio_id = i;//��¼��Ƶ���ı��
    }

    if (g_player_ctx->video_id >= 0) {//����Ƶ��
        g_player_ctx->video_codec_ctx = g_player_ctx->fmt_ctx->streams[g_player_ctx->video_id]->codec;//�õ�������������ָ��
        g_player_ctx->video_codec_ctx->get_buffer = get_buffer_with_pts;
        g_player_ctx->video_codec_ctx->release_buffer = release_buffer_with_pts;
    }
    g_player_ctx->audio_codec_ctx = g_player_ctx->fmt_ctx->streams[g_player_ctx->audio_id]->codec;
    g_player_ctx->audio_codec_ctx->get_buffer = get_buffer_with_pts;
    g_player_ctx->audio_codec_ctx->release_buffer = release_buffer_with_pts;

    AVCodec *audio_codec = avcodec_find_decoder(g_player_ctx->audio_codec_ctx->codec_id);
    printf("the channels : %d\n", g_player_ctx->audio_codec_ctx->channels);

    if (!audio_codec) {
        avformat_close_input(&g_player_ctx->fmt_ctx);
        printf("get audio decodec failed\n");
        return -1;
    }
    if (avcodec_open2(g_player_ctx->audio_codec_ctx, audio_codec, NULL) < 0) {
        avformat_close_input(&g_player_ctx->fmt_ctx);
        printf("initial the audio codec context with codec failed!\n");
        return -1;
    }

    //bytes_per_sec
    g_player_ctx->audio_bytes_per_sec = (g_player_ctx->audio_codec_ctx->channels * 2) 
        * g_player_ctx->audio_codec_ctx->sample_rate;
    printf("audio_bytes_per_seconde : = %d\n", g_player_ctx->audio_bytes_per_sec);

    if (g_player_ctx->video_id < 0)//����Ƶ��
        return 0;

    //�õ�������
    AVCodec *video_codec = avcodec_find_decoder(g_player_ctx->video_codec_ctx->codec_id);

    if (!video_codec) {
        avformat_close_input(&g_player_ctx->fmt_ctx);
        printf("get video decodec failed\n");
        return -1;
    }

    //֪ͨ�����������ܹ�����ضϵ�bit��
    //pCodecCtx->flags |= CODEC_FLAG_TRUNCATED;

    //ʹ��ָ����������ʼ��������������
    if (avcodec_open2(g_player_ctx->video_codec_ctx, video_codec, NULL) < 0) {
        avformat_close_input(&g_player_ctx->fmt_ctx);
        printf("initial the video codec context with codec failed!\n");
        return -1;
    }
    g_player_ctx->fps = 1 / av_q2d(g_player_ctx->fmt_ctx->streams[g_player_ctx->video_id]->r_frame_rate);
    g_player_ctx->video_time_base = av_q2d(g_player_ctx->fmt_ctx->streams[g_player_ctx->video_id]->time_base);
    printf("video_time_base(in s) : = %f\n", g_player_ctx->video_time_base);

    g_player_ctx->width = g_player_ctx->video_codec_ctx->width;
    g_player_ctx->height = g_player_ctx->video_codec_ctx->height;

    return 0;
}

//�ļ�����������Ҫ������Ƶ�ļ���������Ƶ���ݰ�����Ƶ���ݰ�
//���ֱ�������������С�
void* demux(void* filename)
{

    AVPacket *pPacket = NULL;

    while (true) {

        pPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
        av_init_packet(pPacket);
        //fflush(stdout);

        if (av_read_frame(g_player_ctx->fmt_ctx, pPacket) < 0) {
            av_free(pPacket);
            //printf("wrong packet\n");
            continue;
        }

        //av_dup_packet(pPacket);
        if (g_player_ctx->video_id == pPacket->stream_index) {
            push_to_queue(&video_pkt_queue, pPacket);
        } else if (g_player_ctx->audio_id == pPacket->stream_index) {
            push_to_queue(&audio_pkt_queue, pPacket);
        } else {
            av_free_packet(pPacket);
            //av_free(pPacket);
            continue;
        }
    }

    printf("file handler thread exit!\n");
    av_free_packet(pPacket);
    return NULL;
}
