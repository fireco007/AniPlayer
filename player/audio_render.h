/** 
* @file         audio_render.h 
* @Synopsis     audio render struct and function declaration 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/

#ifndef __AUDIO_RENDER_H__
#define __AUDIO_RENDER_H__

extern "C" {
    extern struct queue* audio_sam_queue;
    extern struct player_context* g_player_ctx; 
    void *audio_player(void *);
    double get_current_audio_clock();
    static int pcm_fmt_tran(AVCodecContext *codec_ctx, AVFrame *frame);
}

#endif //__AUDIO_RENDER_H__