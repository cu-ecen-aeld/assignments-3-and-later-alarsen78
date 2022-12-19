#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* args = (struct thread_data*) thread_param;

    usleep(args->wait_obtain*1000);
    const int locked = pthread_mutex_lock(args->mutex);

    args->thread_complete_success = false;
    if (locked == 0)
    {
        usleep(args->wait_release*1000);
        const int unlocked = pthread_mutex_unlock(args->mutex);
        if (unlocked == 0) {
            args->thread_complete_success = true;
        }
    }

    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* data = malloc(sizeof(struct thread_data));
    data->mutex = mutex;
    data->wait_obtain = wait_to_obtain_ms;
    data->wait_release = wait_to_release_ms;
    data->thread_complete_success = false;

    const int result = pthread_create(thread, NULL, threadfunc, (void*)data);
    return (result == 0);
}

