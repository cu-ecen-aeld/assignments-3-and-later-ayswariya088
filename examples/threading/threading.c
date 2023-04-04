#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
/*
Edited for Assignment-4 Part-1
*/
// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg, ...)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

void *threadfunc(void *thread_param)
{
    DEBUG_LOG("starting the threadfunc ");
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data *thread_func_args = (struct thread_data *)thread_param; // obtaining the thread arguments from your parameter
                                                                               //  wait to obtain
    usleep(thread_func_args->wait_to_obtain_ms * 1000); // converting to micro seconds for usleep function
    // obtain mutex
    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc == 0)
    {
        // wait to release
        usleep(thread_func_args->wait_to_release_ms * 1000); // converting to micro seconds for usleep function
        // release the mutex
        rc = pthread_mutex_unlock(thread_func_args->mutex);
        if (rc == 0) // to check if unlocked
        {

            DEBUG_LOG("pthread_mutex_unlock successful");
            thread_func_args->thread_complete_success = true; // make thread_complete_success param true
        }
    }
    else
    {
        // if not successful
        ERROR_LOG("pthread_mutex_lock not successful");
    }

    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    DEBUG_LOG("starting the start_thread_obtaining_mutex");

    struct thread_data *threadParams = (struct thread_data *)malloc(sizeof(struct thread_data));

    if (threadParams == 0)
    {
        ERROR_LOG("threadParams not successfully allocated");
        return false;
    }
    else
    {
        threadParams->mutex = mutex;
        threadParams->wait_to_obtain_ms = wait_to_obtain_ms;
        threadParams->wait_to_release_ms = wait_to_release_ms;
        threadParams->thread_complete_success = false;

        int rc = pthread_create(thread, NULL, threadfunc, threadParams);

        if (rc == 0)
        {
            DEBUG_LOG("Thread successfully started.");
            return true;
        }
        else
        {
            ERROR_LOG("Thread could not be started, Failure occured.");
            return false;
        }
    }
}
