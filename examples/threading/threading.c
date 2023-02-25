#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)


/// @brief 
/// @param thread_param 
/// @return 
void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_args = (struct thread_data *) thread_param;
    float wait_to_obtain = (float) thread_args->wait_to_obtain;
    float wait_to_release = (float) thread_args->wait_to_release;
    
    int ret2;
    //int locked;

    sleep(wait_to_obtain/1000);
    //printf("Locking mutex\n");

    ret2 = pthread_mutex_lock (thread_args->mutex);
    if(ret2!=0)
    {
        thread_args->thread_complete_success=false;
        return thread_param;
    }

    //printf("Wait to unlocking mutex\n");
    sleep(wait_to_release/1000);
    //printf("Unlocking mutex");
    ret2 = pthread_mutex_unlock(thread_args->mutex);
    if(ret2!=0)
    {
        //printf("Mutex failed to unlock\n");
        thread_args->thread_complete_success=false;
        return thread_param;
    }
    
    thread_args->thread_complete_success=true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int ret;
    struct thread_data *input_data = (struct thread_data *) malloc(sizeof(struct thread_data));

    input_data->wait_to_obtain= wait_to_obtain_ms;
    input_data->wait_to_release = wait_to_release_ms;
    input_data->mutex = mutex;

    ret = pthread_create(thread, NULL, threadfunc, input_data);
    //success = input_data->thread_complete_success;
    if(ret==0)
    {
        return true;
    }
    
    return false;
}


