/* Very simple queue
 * These are FIFO queues which discard the new data when full.
 *
 * Queue is empty when in == out.
 * If in != out, then
 *  - items are placed into in before incrementing in
 *  - items are removed from out before incrementing out
 * Queue is full when in == (out-1 + QUEUE_SIZE) % QUEUE_SIZE;
 *
 * The queue will hold QUEUE_ELEMENTS number of items before the
 * calls to QueuePut fail.
 */

#include "cqueue.h"

/* Queue structure */
#define QUEUE_ELEMENTS 1024
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)
#define MAX_PATH_MESSAGE 1024

char Queue[QUEUE_SIZE][MAX_PATH_MESSAGE + 1];
int QueueIn = 0, QueueOut = 0;

int QueuePut(char *msg)
{
    if(QueueIn == (( QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
    {
        return -1; /* Queue Full*/
    }

    strncpy(Queue[QueueIn], msg, MAX_PATH_MESSAGE);

    QueueIn = (QueueIn + 1) % QUEUE_SIZE;

    return 0; // No errors
}

int QueueGet(char *buf, int size)
{
    if(QueueIn == QueueOut)
    {
        return -1; /* Queue Empty - nothing to get*/
    }

    strncpy(buf, Queue[QueueOut], size);

    QueueOut = (QueueOut + 1) % QUEUE_SIZE;

    return 0; // No errors
}
