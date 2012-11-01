/** 
* @file         audio_dec.h 
* @Synopsis     audio decoder struct and function declaration
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/

#ifndef __AUDIO_DEC_H__
#define __AUDIO_DEC_H__

extern "C" {
    extern struct queue* audio_pkt_queue;
    extern struct queue* audio_sam_queue;
    extern struct player_context* g_player_ctx;
    void* audio_decoder(void*);
}

#endif //__AUDIO_DEC_H__
