/** 
* @file         pthread_queue.h 
* @Synopsis     a thread safe queue, one direction link list 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#ifndef __PTHREAD_QUEUE_H__
#define __PTHREAD_QUEUE_H__

#include <pthread.h>

//默认的队列容量为20
#define DEFAULT_CAPABILITY 8 

//队列节点
struct queue_node {
    void* elem;
    queue_node* next;
};

//队列
struct queue {
    queue_node *head; //队头
    queue_node *tail; //队尾
    int capability;   //容量
    int count;        //元素个数
    pthread_mutex_t mutex;
    pthread_cond_t cond_empty;
    pthread_cond_t cond_full;
};

void init_queue(struct queue** pQueue);
void destroy_queue(struct queue** pQueue);
int push_to_queue(struct queue** pQueue, void *elem);
void *pop_from_queue(struct queue **pQueue);
int get_queue_size(struct queue **pQueue);


#endif __PTHREAD_QUEUE_H__
