/** 
* @file         video_render.cpp 
* @Synopsis     render data picture queue to the GTK window 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-9  
*/
#include <Windows.h>
#include <gtk\gtk.h>
#include "video_render.h"
#include "audio_render.h"
extern "C"{
#include "libavutil\time.h"
};

void* video_render(void* userdata)
{
    if (!video_pic_queue)
        return NULL;

    GtkWidget *widget = (GtkWidget*)userdata;
    double av_diff = 0;
    while (TRUE)
    {
        DWORD rest_time = GetTickCount();
        if (widget == NULL)
        {
            continue;
        }

        struct image_data *image_data = (struct image_data*)pop_from_queue(&video_pic_queue);
        cairo_surface_t *image = image_data->image;

        gdk_threads_enter();
        cairo_t *cr = gdk_cairo_create(widget->window);

        cairo_set_source_surface(cr, image, 0, 0);
        cairo_paint(cr);

        cairo_destroy(cr);
        cairo_surface_destroy(image);
        gdk_threads_leave();

        rest_time = GetTickCount() - rest_time;
        rest_time = DWORD(g_player_ctx->fps * 1000) - rest_time;

        //计算帧的延迟时间
        av_diff = get_current_audio_clock() - image_data->pts;

        if (av_diff > 0.2) {//音频播放速度较快，减少延时
            if (av_diff < 0.5) {
                rest_time -= rest_time / 4;
            } else {
                rest_time -= rest_time / 2;
            }
        } else if (av_diff < -0.2) {
            if (av_diff > -0.5) {
                rest_time += rest_time / 4;
            } else {
                rest_time += rest_time / 2;
            }
        }

        printf("rest time : %d\tav_diff : %f\n", rest_time, av_diff);

        
        if (rest_time > 0) {
            if (rest_time > DWORD(g_player_ctx->fps * 1000)) {
                Sleep(DWORD(g_player_ctx->fps * 1000));
            } else {
                Sleep(rest_time);
            }
        }

        //释放内存数据
        free(image_data->data);
        free(image_data);

    }

    return NULL;
}
