/** 
* @file         player_common.cpp 
* @Synopsis      
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include "player_common.h"

struct queue* video_pkt_queue;//��Ƶ���ݰ�����
struct queue* video_pic_queue;//��Ƶͼ�����
struct queue* audio_pkt_queue;
struct queue* audio_sam_queue;

struct player_context* g_player_ctx;
