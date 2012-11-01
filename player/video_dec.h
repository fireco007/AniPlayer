/** 
* @file         video_dec.h 
* @Synopsis      
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#ifndef __VIDEO_DEC_H__
#define __VIDEO_DEC_H__

extern "C" {
    extern struct queue* video_pkt_queue;//视频数据包队列
    extern struct queue* video_pic_queue;//视频图像队列
    extern struct player_context* g_player_ctx;
    void* video_decoder(void *);
}

#endif