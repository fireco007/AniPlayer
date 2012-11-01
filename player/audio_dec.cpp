/** 
* @file         audio_dec.cpp 
* @Synopsis     thread function, decode AVPacket into AVFrame and push to queue 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include <Windows.h>
#include "player_common.h"
#include "audio_dec.h"

void* audio_decoder(void* param)
{
    int got_frame = 0;
    int data_len = 0;
    AVPacket *packet = NULL;
    AVFrame *audio_frame = NULL;
    //av_rescale_q
    while (true) {

        Sleep(20);
        packet = (AVPacket*)pop_from_queue(&audio_pkt_queue);
        if (packet == NULL)
            continue;

        while (true) {
            audio_frame = avcodec_alloc_frame();
            if (!audio_frame)
                return (void*)AVERROR(ENOMEM);

            //һ��packet�����ж����Ƶ��frame�� �����������ѭ�����õ������data_len
            data_len = avcodec_decode_audio4(g_player_ctx->audio_codec_ctx, audio_frame, &got_frame, packet);
            //printf("the frame len is %d\n", data_len);
            if (data_len < 0) {
                printf("decode packet failed\n");
                av_free(audio_frame);
                break;//����������⣬�����˰�
            }

            packet->data += data_len;
            packet->size -= data_len;

            if (!got_frame) {
                if ((audio_frame->data[0]) != NULL)
                    printf("no empty\n");
                av_free(audio_frame);
                break;//�������packet�Ľ����������ǰѭ��
            }

            push_to_queue(&audio_sam_queue, audio_frame);
        }

        //packet������ɣ��ͷ��ڴ�
        av_free(packet);

    }

    return NULL;
}
