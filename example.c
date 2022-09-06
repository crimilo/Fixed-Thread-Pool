#include "thread_pool.h"
#include <stdio.h>

void* foo(void* bar)
{
    (void) bar;
    printf("Hello from thread %lu\n", pthread_self());
    return NULL;
}

int main()
{
    thread_pool_t thread_pool;

    // Initialize the pool with 3 threads and the tasks queue with an initial capacity of 16.
    if (!thread_pool_init(&thread_pool, 3, 16))
    {
        puts("thread_pool_init() failed");
        return -1;
    }

    for (int i = 0; i < 10; i++)
    {
        // Add the foo() function to the thread pool queue, ready to run.
        if (!thread_pool_submit(&thread_pool, &foo, NULL))
        {
            puts("thread_pool_submit() failed");
            return -1;
        }
    }

    // Wait for all the tasks in the queue to run.
    thread_pool_wait(&thread_pool);

    // Frees the thread pool memory and stops it from running.
    thread_pool_destroy(&thread_pool);
}