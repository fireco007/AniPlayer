/** 
* @file         pthread_queue.cpp 
* @Synopsis     the oprate function of thread safe queue 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include <stdlib.h>
#include "pthread_queue.h"

void init_queue(struct queue** pQueue)
{
    *pQueue = (struct queue*)malloc(sizeof(struct queue));
    (*pQueue)->capability = DEFAULT_CAPABILITY;
    (*pQueue)->head = NULL;
    (*pQueue)->tail = NULL;
    pthread_mutex_init(&((*pQueue)->mutex), NULL);//PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&((*pQueue)->cond_empty), NULL);
    pthread_cond_init(&((*pQueue)->cond_full), NULL);
    (*pQueue)->count = 0;
}

void destroy_queue(struct queue** pQueue)
{
    pthread_mutex_destroy(&((*pQueue)->mutex));
    pthread_cond_destroy(&((*pQueue)->cond_empty));
    pthread_cond_destroy(&((*pQueue)->cond_full));
}

int push_to_queue(struct queue** pQueue, void *elem)
{
    if (!elem)
        return 0;//插入为空

    if (!(*pQueue))
        return -1;//队列未分配空间

    struct queue_node *pNode = (struct queue_node*)malloc(sizeof(struct queue_node));
    pNode->elem = elem;
    pNode->next = NULL;

    pthread_mutex_lock(&(*pQueue)->mutex);
    while (((*pQueue)->count) == DEFAULT_CAPABILITY) {//队列满，则阻塞等待条件变量
        pthread_cond_wait(&(*pQueue)->cond_full, &(*pQueue)->mutex);
    }

    if ((*pQueue)->count == 0) {//空队列
        (*pQueue)->tail = pNode;
        (*pQueue)->head = pNode;
    } else {//尾部插入元素
        (*pQueue)->tail->next = pNode;
        (*pQueue)->tail = pNode;
    }
    
    (*pQueue)->count++;
    pthread_mutex_unlock(&(*pQueue)->mutex);
    pthread_cond_signal(&(*pQueue)->cond_empty);
    return 0;
}

void* pop_from_queue(struct queue **pQueue)
{
    if (!(*pQueue))
        return NULL;

    struct queue_node *pNode;

    pthread_mutex_lock(&(*pQueue)->mutex);
    while ((*pQueue)->count == 0) //队列空，则阻塞等待条件变量
        pthread_cond_wait(&(*pQueue)->cond_empty, &(*pQueue)->mutex);
    if ((*pQueue)->count == 1) {
        pNode = (*pQueue)->head;
        (*pQueue)->head = NULL;
        (*pQueue)->tail = NULL;
    } else {//头部弹出元素
        pNode = (*pQueue)->head;
        (*pQueue)->head = pNode->next;
    }

    (*pQueue)->count--;
    pthread_mutex_unlock(&(*pQueue)->mutex);
    pthread_cond_signal(&(*pQueue)->cond_full);

    void *ret = pNode->elem;
    free(pNode);
    return ret;
}

int get_queue_size(struct queue **pQueue)
{
    int count;
    if ((*pQueue) == NULL)
        return 0;

    pthread_mutex_lock(&(*pQueue)->mutex);
    count = (*pQueue)->count;
    pthread_mutex_unlock(&(*pQueue)->mutex);
    return count;
}
