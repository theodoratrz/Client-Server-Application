#include "../include/queueList.h"

struct queuelist{
    ListNode headNode;  // dummy node for easier iterations/insert/delete
    ListNode lastNode;
    int listSize;
};

struct listNode{
    ListNode nextNode;
    void* value;
};

QueueList createList(){
    QueueList q = malloc(sizeof(struct queuelist));
    if (q == NULL){
        printf("Memory allocation failed");
        return -1;
    }
    ListNode fNode = malloc(sizeof(fNode));
    if (fNode == NULL){
        printf("Memory allocation failed");
        return -1;
    }
    // make a head node
    q->headNode = fNode;
    q->lastNode = q->headNode;
    q->headNode->nextNode = NULL;
    q->listSize = 0;

    return q;
}

// delete each node and the struct
void destroyList(QueueList q){
    for (ListNode node = q->headNode->nextNode; node != NULL; node = node->nextNode){
    ListNode next = node->nextNode;
    free(node);
    node = next;
    }

     free(q);
}

int getListSize(QueueList q){
    return q->listSize;
}

int push(QueueList q, void* value) {
    ListNode n = malloc(sizeof(*n));
    if (n == NULL) {
        perror("Memory allocation failed");
        return -1;
    }
    n->value = value;
    n->nextNode = NULL;

    if (q->lastNode != NULL) {
        q->lastNode->nextNode = n;
    } else {
        q->headNode->nextNode = n;
    }
    q->lastNode = n;
    q->listSize++;
    return 0;
}

int pop(QueueList q) {
    if (q->listSize == 0) {
        return -1;
    }

    ListNode n = q->headNode->nextNode;
    if (n != NULL) {
        q->headNode->nextNode = n->nextNode;
        if (q->lastNode == n) {
            q->lastNode = NULL;
        }
        q->listSize--;
        free(n);
        return 0;
    }

    return -1;
}

int removeValue(QueueList q, void* value, Compare compare) {
    ListNode previous = q->headNode;
    ListNode exists = find(q, value, compare);  // check if it exists
    if (exists == NULL)
        return -1;

    ListNode node = q->headNode->nextNode;
    while (node != NULL) {
        if (compare(value, node->value) == 0) {
            previous->nextNode = node->nextNode;    // link previous and next node
            if (node == q->lastNode) {
                q->lastNode = previous; // update lastNode if necessary
            }
            q->listSize--;
            free(node);
            return 0;
        }
        previous = node;
        node = node->nextNode;
    }
    return -1;
}


ListNode find(QueueList q, void* value, Compare compare){
    for (ListNode node = q->headNode->nextNode; node != NULL; node = node->nextNode){
        if(compare(value, node->value) == 0)    // based on compare function find a value
            return node;
    }
    return NULL;
}

// functions for accessing nodes

ListNode getListFirst(QueueList q){
    return q->headNode->nextNode;
}

ListNode getListLast(QueueList q){
    return q->lastNode;
}

ListNode getListNext(QueueList q, ListNode n){
    return n->nextNode;
}

void* getNodeValue(QueueList q, ListNode n){
    return n->value;
}

ListNode createNode(void* value){
    ListNode newNode = malloc(sizeof(*newNode));
    if (newNode == NULL){
        printf("Memory allocation failed");
        return NULL;
    }
    newNode->value = value;
    newNode->nextNode = NULL;

    return newNode;
}