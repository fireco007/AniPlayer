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

//Ĭ�ϵĶ�������Ϊ20
#define DEFAULT_CAPABILITY 8 

//���нڵ�
struct queue_node {
    void* elem;
    queue_node* next;
};

//����
struct queue {
    queue_node *head; //��ͷ
    queue_node *tail; //��β
    int capability;   //����
    int count;        //Ԫ�ظ���
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
