#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdint.h>

typedef struct thread_pool_queue thread_pool_queue_t;
typedef struct thread_pool_task thread_pool_task_t;
typedef struct thread_pool thread_pool_t;

/*
* Implemented as a circular array queue.
*/
struct thread_pool_queue
{
    thread_pool_task_t* tasks;
    int32_t front;
    int32_t rear;
    int32_t capacity;
};

struct thread_pool_task
{
    void* (*function)(void*);
    void* argument;
};

struct thread_pool
{
    thread_pool_queue_t pending_tasks;
    pthread_mutex_t mutex;
    pthread_cond_t condition_loop; // Used to make threads in the pool wait if the task queue is empty.
    pthread_cond_t condition_wait; // Used to make the calling thread wait for all tasks in the queue to complete.
    pthread_t* threads;
    uint8_t threads_count;
    uint8_t idle_threads;
    int8_t is_active;
};

/*
* @brief Initialize the thread pool and start execution.
*
* @param threads_count Fixed number of threads in the pool.
* @param queue_initial_capacity Initial capacity of the task queue.
*
* @return 1 on success, or 0 on failure.
*/
int thread_pool_init(thread_pool_t* thread_pool, uint8_t threads_count, uint32_t queue_initial_capacity);

/*
* @brief Frees the thread pool memory and immediately stops it.
*/
void thread_pool_destroy(thread_pool_t* thread_pool);

/*
* @brief Sends the function to the thread pool, waiting for a thread to fetch and execute it.
* If the task queue is full, a grow operation is performed.
*
* @return 1 on success, or 0 if the queue grow operation fails.
*/
int thread_pool_submit(thread_pool_t* thread_pool, void* (*function)(void*), void* argument);

/*
* @brief Wait for all queued tasks to finish running.
*/
void thread_pool_wait(thread_pool_t* thread_pool);

#endif