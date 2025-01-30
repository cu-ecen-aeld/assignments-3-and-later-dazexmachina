#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* data = (struct thread_data*) thread_param;
    sleep(data->wait_obtain_ms / 1000);
    pthread_mutex_lock(data->sleep_lock);
    sleep(data->wait_release_ms / 1000);
    pthread_mutex_unlock(data->sleep_lock);
    pthread_mutex_lock(data->status_lock);
    data->thread_complete_success = true;
    pthread_mutex_unlock(data->status_lock);

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
    data->sleep_lock = mutex;
    data->status_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(data->status_lock, NULL);
    data->wait_obtain_ms = wait_to_obtain_ms;
    data->wait_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    DEBUG_LOG("creating thread");

    if (pthread_create(thread, NULL, threadfunc, data) != 0) {
        ERROR_LOG("could not create thread");
        pthread_mutex_destroy(data->status_lock);
        free(data);
        
        return false;
    }

    pthread_mutex_destroy(data->status_lock);
    return true;

    /*while(true) {
        DEBUG_LOG("checking for completion...");
        pthread_mutex_lock(data->status_lock);
        bool complete = data->thread_complete_success;
        
        if (complete) {
            DEBUG_LOG("completed");
        }
        else {
            DEBUG_LOG("in progress");
        }
        pthread_mutex_unlock(data->status_lock);

        if (complete) {
            if (pthread_join(*thread, (void**)&data) != 0) {
                ERROR_LOG("could not join thread");
            }
            pthread_mutex_destroy(data->status_lock);
            free(data);

            return data->thread_complete_success;
        }
        sleep(1);
    }

    return false;
*/
}
