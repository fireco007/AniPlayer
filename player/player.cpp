/** 
* @file         player.cpp 
* @Synopsis     main function and gtk window  
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include <gtk/gtk.h>
#include <stdio.h>
//#include <signal.h>

#include "player_common.h"
#include "file_handler.h"
#include "video_dec.h"
#include "audio_dec.h"
#include "audio_render.h"
#include "video_render.h"
#include "pthread_queue.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


//���������䣬�ڴ��ں��潫�����һ������̨����
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

extern "C" {
    extern struct queue* video_pkt_queue;//��Ƶ���ݰ�����
    extern struct queue* video_pic_queue;//��Ƶͼ�����
    extern struct queue* audio_pkt_queue;
    extern struct queue* audio_sam_queue;
    extern struct player_context* g_player_ctx;
}

//for test
const char test_file_list[][50] =
{
    "moonLightStone.flac",//0
    "Yuruyuri04.mp4",     //1
    "Guilty_Crown07.mp4", //2
    "Guilty_Crown07.mkv", //3
    "anamnesis.mp3",      //4
    "yakusoku.ape",       //5
    "Guilty_Crown11.mp4", //6
    "Chuunibyou01.mp4"    //7
};

const char *filename = test_file_list[5];

static GdkRectangle rect;//������ʾvideo���������RGBͼ��֡������
static pthread_t file_tid, dec_tid;
static pthread_t audio_dec_tid, audio_render_tid, video_render_tid; //���߳�ID
guint  time_out;

gboolean on_darea_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data);


gboolean on_timer(GtkWidget *widget)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//�����̷߳�������

    if (0 != pthread_create(&video_render_tid, &attr, video_render, widget))
        printf("create video render thread failed!\n");

    gtk_timeout_remove(time_out);

    //gdk_window_invalidate_rect(widget->window, &rect, TRUE);
    return TRUE;
}

gint exit_clean(GtkWidget *widget, gpointer data)
{
    pthread_cancel(file_tid);
    pthread_cancel(dec_tid);
    pthread_cancel(audio_dec_tid);
    pthread_cancel(audio_render_tid);

    avcodec_close(g_player_ctx->audio_codec_ctx);
    if (g_player_ctx->video_id >= 0)
        avcodec_close(g_player_ctx->video_codec_ctx);

    avformat_close_input(&g_player_ctx->fmt_ctx);
    pthread_mutex_destroy(&g_player_ctx->audio_time_mutex);
    free(g_player_ctx);
    gtk_main_quit();
    return 0;
}


int main(int argc, char **argv)
{
    if (initDecoderWithFile(filename))
        return -1;

    init_queue(&video_pkt_queue); //��ʼ����Ƶ���ݰ�����
    init_queue(&video_pic_queue); //��ʼ����Ƶͼ�����
    init_queue(&audio_pkt_queue);
    init_queue(&audio_sam_queue);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//�����̷߳�������

    //�ļ������߳�
    if (0 != pthread_create(&file_tid, &attr, demux, (void*)filename))
        printf("create file handler thread failed!\n");

    //��Ƶ�����߳�
    if (0 != pthread_create(&dec_tid, &attr, video_decoder, NULL))
        printf("create decode thread failed!\n");

    if (0 != pthread_create(&audio_dec_tid, &attr, audio_decoder, NULL))
        printf("create audio decode thread failed!\n");

    if (0 != pthread_create(&audio_render_tid, &attr, audio_player, NULL))
        printf("create audio render thread failed!\n");

    /*������Gtk������ش���*/
    if (!g_thread_supported())
        g_thread_init(NULL);

    gdk_threads_init();
    gdk_threads_enter();

    GtkWidget *main_win;//������
    //GtkWidget *drawable;//ͼ���������
    gtk_init(&argc, &argv);

    char win_title[100];
    sprintf(win_title, "AniPlayer--by Excalibur : %s", filename);
    main_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_win), win_title);
    GtkWidget *drawable = gtk_drawing_area_new();

    gtk_container_add(GTK_CONTAINER(main_win), drawable);
    g_signal_connect(G_OBJECT(main_win), "delete-event", G_CALLBACK(exit_clean), NULL);
    //gtk_signal_connect(GTK_OBJECT(drawable), "expose-event", G_CALLBACK(on_darea_expose), NULL);

    gtk_widget_show_all(main_win);

    gtk_widget_set_size_request(drawable, g_player_ctx->width, g_player_ctx->height);

    //�ر��Զ�ˢ�º�˫����
    gtk_widget_set_app_paintable(drawable, TRUE);
    gtk_widget_set_double_buffered(drawable, FALSE);//���˴�ӣ�֮ǰûdisable��һֱˢ����

    time_out = gtk_timeout_add(40, GSourceFunc(on_timer), drawable);

    gtk_main();

    gdk_threads_leave();
    
    _CrtDumpMemoryLeaks();

    //getchar();
    return 0;
}



gboolean on_darea_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    rect.x = event->area.x;
    rect.y = event->area.y;
    rect.height = event->area.height;
    rect.width = event->area.width;
    if (!video_pic_queue)
        return TRUE;

    if ((video_pic_queue->count) == 0)
        return TRUE;

    //AVFrame *frame = (AVFrame*)pop_from_queue(&video_pic_queue);//�����������
    //
    //guchar *rgbbuff = (guchar*)(frame->data[0]);

    ////����ͼ�ε��ڴ�ӳ��
    //cairo_surface_t *image = cairo_image_surface_create_for_data(rgbbuff, CAIRO_FORMAT_ARGB32, g_player_ctx->width, 
    //    g_player_ctx->height, g_player_ctx->width * 4);

    struct image_data *image_data = (struct image_data*)pop_from_queue(&video_pic_queue);
    cairo_surface_t *image = image_data->image;

    cairo_t *cr = gdk_cairo_create(widget->window);

    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(image);

    free(image_data->data);
    free(image_data);

    //gdk_draw_rgb_image(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], 0, 0, g_player_ctx->width, 
    //    g_player_ctx->height, GDK_RGB_DITHER_MAX, rgbbuff, g_player_ctx->width * 3);


    ////�ͷ�ǰһ���ڵ�
    //if (g_prev_frame) {
    //    free(frame->data[0]);
    //    av_free(frame);
    //}

    //g_prev_frame = frame;

    return TRUE;
}
