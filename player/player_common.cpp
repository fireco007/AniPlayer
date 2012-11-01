/** 
* @file         player_common.cpp 
* @Synopsis      
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include "player_common.h"

struct queue* video_pkt_queue;//视频数据包队列
struct queue* video_pic_queue;//视频图像队列
struct queue* audio_pkt_queue;
struct queue* audio_sam_queue;

struct player_context* g_player_ctx;
