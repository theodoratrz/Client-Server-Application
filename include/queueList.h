#ifndef QUEUELIST_H
#define QUEUELIST_H

#include <stdio.h>
#include <stdlib.h>

// data structure used for buffer. It resembles a queue because you push at the end and pop at the front (fifo)
// but has an extra feature to remove a value from anywhere (in case of stop)
typedef struct queuelist* QueueList;
typedef struct listNode* ListNode;
typedef struct Job {
    int jobId;
    char jobDescr[256];
    int clientSocket;
} Job;

typedef int (*Compare) (void* a, void* b);    // compare function

QueueList createList();
ListNode createNode(void* v);
int getListSize(QueueList q);
int push(QueueList q, void* v);
int pop(QueueList q);
int removeValue(QueueList q, void* value, Compare func);
ListNode find(QueueList q, void* v, Compare func);
void destroyList(QueueList q);
ListNode getListFirst(QueueList q);
ListNode getListLast(QueueList q);
ListNode getListNext(QueueList q, ListNode n);
void* getNodeValue(QueueList q, ListNode n);

#endif