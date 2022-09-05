#include "thread_pool.h"
#include <stdlib.h>

thread_pool_queue_t* queue_init(thread_pool_queue_t* queue, int32_t initial_capacity)
{
    if (initial_capacity < 1)
        return NULL;

    queue->tasks = (thread_pool_task_t*) malloc(sizeof(thread_pool_task_t) * initial_capacity);

    if (!queue->tasks)
        return NULL;

    queue->front = -1;
    queue->rear = -1;
    queue->capacity = initial_capacity;

    return queue;
}

void queue_destroy(thread_pool_queue_t* queue)
{
    free(queue->tasks);
}

int queue_is_empty(const thread_pool_queue_t* queue)
{
    return queue->front == -1;
}

int queue_is_full(const thread_pool_queue_t* queue)
{
    return (queue->rear + 1) % queue->capacity == queue->front;
}

thread_pool_queue_t* queue_grow(thread_pool_queue_t* queue)
{
    thread_pool_task_t* new_tasks = (thread_pool_task_t*) malloc(sizeof(thread_pool_task_t) * queue->capacity * 2);

    if (!new_tasks)
        return NULL;

    int32_t i;

    for (i = 0; queue->front != queue->rear; i++)
    {
        new_tasks[i] = queue->tasks[queue->front];
        queue->front = (queue->front + 1) % queue->capacity;
    }

    new_tasks[i] = queue->tasks[queue->front];

    queue->front = 0;
    queue->rear = i;
    queue->capacity *= 2;

    free(queue->tasks);
    queue->tasks = new_tasks;

    return queue;
}

thread_pool_task_t* queue_enqueue(thread_pool_queue_t* queue, thread_pool_task_t* task)
{
    if (queue_is_full(queue))
    {
        if (!queue_grow(queue))
            return NULL;
    }

    if (queue_is_empty(queue))
        queue->front = queue->rear = 0;
    else
        queue->rear = (queue->rear + 1) % queue->capacity;

    queue->tasks[queue->rear] = *task;

    return task;
}

thread_pool_task_t* queue_dequeue(thread_pool_queue_t* queue)
{
    thread_pool_task_t* removed = (thread_pool_task_t*) malloc(sizeof(thread_pool_task_t));

    if (!removed)
        return NULL;

    *removed = queue->tasks[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;

    if (queue->front == (queue->rear + 1) % queue->capacity)
    {
        // The queue has emptied, reset front and rear.
        queue->front = -1;
        queue->rear = -1;
    }

    return removed;
}

void* thread_loop(thread_pool_t* thread_pool)
{
    while (1)
    {
        pthread_mutex_lock(&thread_pool->mutex);

        if (!thread_pool->is_active)
        {
            pthread_mutex_unlock(&thread_pool->mutex);
            break;
        }

        while (queue_is_empty(&thread_pool->pending_tasks))
        {
            thread_pool->idle_threads++;

            if (thread_pool->idle_threads == thread_pool->threads_count)
            {
                // Wakes up thread_pool_wait().
                pthread_cond_signal(&thread_pool->condition_wait);
            }

            pthread_cond_wait(&thread_pool->condition_loop, &thread_pool->mutex);
            thread_pool->idle_threads--;

            if (!thread_pool->is_active)
            {
                pthread_mutex_unlock(&thread_pool->mutex);
                return NULL;
            }
        }

        thread_pool_task_t* task = queue_dequeue(&thread_pool->pending_tasks);

        if (queue_is_empty(&thread_pool->pending_tasks))
            pthread_cond_signal(&thread_pool->condition_wait);

        pthread_mutex_unlock(&thread_pool->mutex);

        task->function(task->argument);
        free(task);
    }
    return NULL;
}

int thread_pool_init(thread_pool_t* thread_pool, uint8_t threads_count, uint32_t queue_initial_capacity)
{
    if (threads_count < 1)
        return 0;

    if (!queue_init(&thread_pool->pending_tasks, queue_initial_capacity))
        return 0;

    thread_pool->threads_count = 0;
    thread_pool->idle_threads = 0;

    if (pthread_mutex_init(&thread_pool->mutex, NULL) || 
        pthread_cond_init(&thread_pool->condition_loop, NULL) ||
        pthread_cond_init(&thread_pool->condition_wait, NULL))
    {
        thread_pool_destroy(thread_pool);
        return 0;
    }

    thread_pool->threads = (pthread_t*) malloc(sizeof(pthread_t) * threads_count);

    if (!thread_pool->threads)
    {
        thread_pool_destroy(thread_pool);
        return 0;
    }

    thread_pool->is_active = 1;

    for (int i = 0; i < threads_count; i++)
    {
        if (pthread_create(&thread_pool->threads[i], NULL, (void* (*)(void*)) &thread_loop, thread_pool))
        {
            thread_pool_destroy(thread_pool);
            return 0;
        }

        thread_pool->threads_count = i;
    }

    return 1;
}

void thread_pool_destroy(thread_pool_t* thread_pool)
{
    pthread_mutex_lock(&thread_pool->mutex);
    thread_pool->is_active = 0;
    pthread_mutex_unlock(&thread_pool->mutex);

    pthread_cond_broadcast(&thread_pool->condition_loop);

    for (int i = 0; i < thread_pool->threads_count; i++)
        pthread_join(thread_pool->threads[i], NULL);

    queue_destroy(&thread_pool->pending_tasks);
    pthread_mutex_destroy(&thread_pool->mutex);
    pthread_cond_destroy(&thread_pool->condition_loop);

    free(thread_pool->threads);
}

int thread_pool_submit(thread_pool_t* thread_pool, void* (*function)(void*), void* argument)
{
    thread_pool_task_t task = { function, argument };

    pthread_mutex_lock(&thread_pool->mutex);
    void* success = queue_enqueue(&thread_pool->pending_tasks, &task);
    pthread_mutex_unlock(&thread_pool->mutex);

    if (success)
    {
        pthread_cond_signal(&thread_pool->condition_loop);
        return 1;
    }

    return 0;
}

void thread_pool_wait(thread_pool_t* thread_pool)
{
    pthread_mutex_lock(&thread_pool->mutex);

    while (!queue_is_empty(&thread_pool->pending_tasks))
        pthread_cond_wait(&thread_pool->condition_wait, &thread_pool->mutex);

    pthread_mutex_unlock(&thread_pool->mutex);
}