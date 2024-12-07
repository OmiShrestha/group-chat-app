#include "mutexes.h"

// Mutexes
pthread_mutex_t userList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t messageList_mutex = PTHREAD_MUTEX_INITIALIZER;

// Destory mutexes
void destroy_mutexes() {
    pthread_mutex_destroy(&userList_mutex);
    pthread_mutex_destroy(&messageList_mutex);
}