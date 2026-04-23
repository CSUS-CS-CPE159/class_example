/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Simple circular queue implementation
 */
#ifndef QUEUE_H
#define QUEUE_H

#include <spede/stdbool.h>

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 20 
#endif

typedef struct q_t {
    int head;
    int tail;
    int size;
    int items[QUEUE_SIZE];
} q_t;

/**
 * Initializes an empty queue
 * Sets the empty queue items to -1
 *
 * @param  queue - pointer to the queue
 * @return -1 on error; 0 on success
 */
int queue_init(q_t *queue);

/**
 * Adds an item to the end of a queue
 * @param  queue - pointer to the queue
 * @param  item  - the item to add
 * @return -1 on error; 0 on success
 */
int queue_in(q_t *queue, int item);

/**
 * Pulls an item from the specified queue
 * @param  queue - pointer to the queue
 * @param  item  - pointer to the memory to save item to
 * @return -1 on error; 0 on success
 */
int queue_out(q_t *queue, int *item);

/**
 * Indicates if the queue is empty
 * @param queue - pointer to the queue structure
 * @return true if empty, false if not empty
 */
bool queue_is_empty(q_t *queue);

/**
 * Indicates if the queue if full
 * @param queue - pointer to the queue structure
 * @return true if full, false if not full
 */
bool queue_is_full(q_t *queue);


#endif
