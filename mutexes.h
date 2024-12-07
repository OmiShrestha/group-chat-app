#define MUTEXES_H

#include <pthread.h>

// Function prototypes
void destroy_mutexes();

// Mutexes for thread synchronization
extern pthread_mutex_t userList_mutex;
extern pthread_mutex_t messageList_mutex;