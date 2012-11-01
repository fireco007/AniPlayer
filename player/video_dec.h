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
    extern struct queue* video_pkt_queue;//��Ƶ���ݰ�����
    extern struct queue* video_pic_queue;//��Ƶͼ�����
    extern struct player_context* g_player_ctx;
    void* video_decoder(void *);
}

#endif