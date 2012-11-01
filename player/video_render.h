/** 
* @file         video_render.h 
* @Synopsis     declaration of video render function and struct 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-9  
*/
#ifndef __VIDEO_RENDER_H__
#define __VIDEO_RENDER_H__

#include "player_common.h"
#include <gtk/gtk.h>

extern "C" {
    extern struct queue* video_pic_queue;
    void* video_render(void*);
}

#endif //__VIDEO_RENDER_H__