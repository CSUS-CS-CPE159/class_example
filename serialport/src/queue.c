/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Simple circular queue implementation
 */
#include "spede.h"
#include "queue.h"

/**
 * Initializes an empty queue
 * Sets the empty queue items to -1
 *
 * @param  queue - pointer to the queue
 * @return -1 on error; 0 on success
 */
int queue_init(q_t *queue) {
    if (!queue) {
        return -1;
    }

    for (int i = 0; i < QUEUE_SIZE; i++) {
        queue->items[i] = 0;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;

    return 0;
}

/**
 * Adds an item to the end of a queue
 * @param  queue - pointer to the queue
 * @param  item  - the item to add
 * @return -1 on error; 0 on success
 */
int queue_in(q_t *queue, int item) {
    if (!queue) {
        return -1;
    }

    // Return an error if the queue is full
    if (queue->size == QUEUE_SIZE) {
	printf("queue is full\n");
    	    return -1;
    }

    // Add the item to the tail of the queue
    queue->items[queue->tail] = item;

    // Move the tail forward
    queue->tail++;

    // If we are at the end of the array, move the tail to the beginning
    if (queue->tail == QUEUE_SIZE) {
        queue->tail = 0;
    }

    // Increment size (since we just added an item to the queue)
    queue->size++;

    return 0;
}

/**
 * Pulls an item from the specified queue
 * @param  queue - pointer to the queue
 * @return -1 on error; 0 on success
 */
int queue_out(q_t *queue, int *item) {
    if (!queue || !item) {
        return -1;
    }

    // return -1 if queue is empty
    if (queue->size == 0) {
        return -1;
    }

    // Get the item from the head of the queue
    *item = queue->items[queue->head];

    // Reset the empty item
    queue->items[queue->head] = 0;

    // Move the head forward
    queue->head++;

    // If we are at the end of the array, move the head to the beginning
    if (queue->head == QUEUE_SIZE) {
        queue->head = 0;
    }

    // Decrement size (since we just removed an item from the queue)
    queue->size--;

    return 0;
}

bool queue_is_empty(q_t *queue){
    return queue->size == 0? true : false;
}

bool queue_is_full(q_t *queue){
    return queue->size == QUEUE_SIZE? true : false;
}
