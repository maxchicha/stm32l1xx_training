#ifndef NBQUEUE_H_
#define NBQUEUE_H_

#include "stm32l1xx.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define QUEUE_SIZE 200

typedef struct NBQueue {
	uint8_t buffer[QUEUE_SIZE];
	uint16_t front;
	uint16_t rear;
	uint16_t itemCount;
} NBQueue;


void NBQueueInit(NBQueue* q);
uint8_t NBQueuePeek(NBQueue* q);
bool NBQueueIsEmpty(NBQueue* q);
bool NBQueueIsFull(NBQueue* q);
uint16_t NBQueueSize(NBQueue* q);
void NBQueueInsert(NBQueue* q, uint8_t data);
uint8_t NBQueueRemove(NBQueue* q);
#endif