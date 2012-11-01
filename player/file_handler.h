/** 
* @file         file_handler.h 
* @Synopsis      
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

extern "C" {
    void* demux(void* filename);
    int initDecoderWithFile(const char* filename);
    extern struct player_context* g_player_ctx; 
    extern struct queue* video_pkt_queue;//视频数据包队列
    extern struct queue* video_pic_queue;//视频图像队列
    extern struct queue* audio_pkt_queue;
    extern struct queue* audio_sam_queue;
}

#endif