/** 
* @file         video_dec.cpp 
* @Synopsis     tranform the AVPacket(video) into cairo_surface_t and push to queue   
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include "video_dec.h"
#include "player_common.h"

extern "C" {
#include <libswscale/swscale.h>
}

double synchronize_video(AVFrame *frame, double pts)
{
    double frame_delay;
    if (pts != 0) {
        g_player_ctx->video_clock = pts;
    } else {
        pts = g_player_ctx->video_clock;
    }

    frame_delay = g_player_ctx->video_time_base;
    frame_delay += frame->repeat_pict * (frame_delay * 0.5);
    g_player_ctx->video_clock += frame_delay;
    return pts;
}

void YUV2BGR0(AVCodecContext *pAVCodecCtx, AVFrame *pSrcFrame, AVFrame *pRGBFrame)
{
    static struct SwsContext *img_convert_ctx = NULL;
    img_convert_ctx = sws_getCachedContext(img_convert_ctx, pAVCodecCtx->width,
        pAVCodecCtx->height,
        pAVCodecCtx->pix_fmt,
        pAVCodecCtx->width,
        pAVCodecCtx->height,
        PIX_FMT_BGR0,//�����ʽ�պú�Cairo�������������
        SWS_BICUBIC,
        NULL,
        NULL,
        NULL);
    sws_scale(img_convert_ctx, pSrcFrame->data, pSrcFrame->linesize, 0, 
        pAVCodecCtx->height, pRGBFrame->data, pRGBFrame->linesize);
}

void* video_decoder(void *)
{
    //todo: �����ڴ������ͷ�Ƶ�������ܷ��Ż�

    AVFrame *pYUVFrame, *pRGBFrame;
    AVPacket *pkt = NULL;
    int iGotPicture = 0;
    double pts = 0;

    if (g_player_ctx->video_id < 0)
        return NULL;

    while (true) {
        pkt = (AVPacket*)pop_from_queue(&video_pkt_queue);
        if (!pkt)
            continue;

        pYUVFrame = avcodec_alloc_frame();
        pRGBFrame = avcodec_alloc_frame();
        pRGBFrame->linesize[0] = g_player_ctx->width * 4;
        pRGBFrame->data[0] = (uint8_t *)malloc((g_player_ctx->width) * (g_player_ctx->height) * 4 * sizeof(uint8_t));
        

        avcodec_decode_video2(g_player_ctx->video_codec_ctx, pYUVFrame, &iGotPicture, pkt);

        //��ȡ��֡��pts(��ʾʱ���)
        if (pkt->dts == AV_NOPTS_VALUE && pYUVFrame->opaque 
            && *(uint64_t*)pYUVFrame->opaque != AV_NOPTS_VALUE) {
            pts = *(uint64_t*)pYUVFrame->opaque;
        } else if (pkt->dts != AV_NOPTS_VALUE) {
            pts = pkt->dts;//packet���Դ���ʾʱ���
        } else {
            pts = 0;
        }

        pts *= g_player_ctx->video_time_base;

        if (iGotPicture) {
            pts = synchronize_video(pYUVFrame, pts);

            YUV2BGR0(g_player_ctx->video_codec_ctx, pYUVFrame, pRGBFrame);

            cairo_surface_t *image = cairo_image_surface_create_for_data(pRGBFrame->data[0], CAIRO_FORMAT_ARGB32, g_player_ctx->width, 
                g_player_ctx->height, g_player_ctx->width * 4);

            struct image_data* image_data = (struct image_data*)malloc(sizeof(struct image_data));

            image_data->image = image;
            image_data->data = pRGBFrame->data[0];
            image_data->pts = pts;

            //�����������RGB24ͼ��֡������Ƶͼ�����
            push_to_queue(&video_pic_queue, image_data);

        }

        //�ͷŴ洢YUV420P��ʽ��ͼ��֡
        av_free(pYUVFrame);
        av_free(pRGBFrame);

        //�ͷ�node�ڵ�packet�Լ�node����
        av_free_packet(pkt);
    }

    printf("decode thread exit!\n");
    
    return NULL;
}
