#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

// Task function type
typedef void (*task_func_t)(void *arg);

// Task structure
typedef struct task {
    task_func_t function;
    void *arg;
    struct task *next;
} task_t;

// Thread pool structure
typedef struct {
    pthread_t *threads;
    int thread_count;
    task_t *task_queue_head;
    task_t *task_queue_tail;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t queue_empty_cond;
    bool shutdown;
    int active_tasks;
} thread_pool_t;

// Initialize thread pool
thread_pool_t* thread_pool_create(int num_threads);

// Submit task to thread pool
int thread_pool_submit(thread_pool_t *pool, task_func_t function, void *arg);

// Wait for all tasks to complete
void thread_pool_wait(thread_pool_t *pool);

// Destroy thread pool
void thread_pool_destroy(thread_pool_t *pool);

#endif // THREAD_POOL_H
