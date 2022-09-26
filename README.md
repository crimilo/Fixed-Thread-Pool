# Fixed Thread Pool
A simple fixed thread pool written in C (with the POSIX thread library).\
The tasks queue is implemented as a circular array queue that automatically grows in size if it is filled.

## Example
Error checking was avoided for the sake of this example. Check `example.c` for a better alternative.

```c
#include "thread_pool.h"
#include <stdio.h>

void* foo(void* bar)
{
    printf("Hello from thread %lu\n", pthread_self());
}

int main()
{
    thread_pool_t thread_pool;

    // Initialize the pool with 3 fixed threads and 16 initial queue capacity.
    thread_pool_init(&thread_pool, 3, 16);

    // Run the foo() function ten times from threads in the pool.
    for (int i = 0; i < 10; i++)
        thread_pool_submit(&thread_pool, &foo, NULL);

    // Wait for all the threads to finish.
    thread_pool_wait(&thread_pool);

    // Frees memory and release resources back to the OS.
    thread_pool_destroy(&thread_pool);
}
```

## How to run
This library uses POSIX threads, therefore you must compile with the `-pthread` flag.\
For example, to compile `example.c`, you would do the following:
`cc example.c thread_pool.c -pthread`.