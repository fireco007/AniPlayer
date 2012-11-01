/** 
* @file         player_common.h 
* @Synopsis     important struct for this player, used in several files 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#ifndef __PLAYER_COMMON_H__
#define __PLAYER_COMMON_H__
#include <gtk\gtk.h>
#include "pthread_queue.h"

extern "C" {
#include <avcodec.h>
#include <avformat.h>
#include <avutil.h>


extern struct queue* video_pkt_queue;//视频数据包队列
extern struct queue* video_pic_queue;//视频图像队列
extern struct queue* audio_pkt_queue;
extern struct queue* audio_sam_queue;

struct player_context {
    AVFormatContext *fmt_ctx;
    AVCodecContext *audio_codec_ctx;
    AVCodecContext *video_codec_ctx;
    double video_clock;
    double audio_clock;
    double video_time_base;
    double fps;
    int audio_bytes_per_sec;
    int video_id;
    int audio_id;
    int width;
    int height;
    pthread_mutex_t audio_time_mutex;//用音频同步视频，音频时钟多线程访问
};

struct image_data {
    cairo_surface_t *image;
    uint8_t *data;
    double pts;
};

extern struct player_context* g_player_ctx;

}

#endif //__PLAYER_COMMON_H__
