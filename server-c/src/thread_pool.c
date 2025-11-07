#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "thread_pool.h"
#include "logger.h"

static void* thread_pool_worker(void *arg);

thread_pool_t* thread_pool_create(int num_threads) {
    if (num_threads <= 0) {
        log_error("잘못된 스레드 개수: %d", num_threads);
        return NULL;
    }

    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (pool == NULL) {
        log_error("스레드 풀 메모리 할당 실패");
        return NULL;
    }

    pool->thread_count = num_threads;
    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->shutdown = false;
    pool->active_tasks = 0;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);
    pthread_cond_init(&pool->queue_empty_cond, NULL);

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool) != 0) {
            log_error("스레드 생성 실패 %d", i);
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    log_info("%d개의 스레드로 스레드 풀 생성 완료", num_threads);
    return pool;
}

int thread_pool_submit(thread_pool_t *pool, task_func_t function, void *arg) {
    if (pool == NULL || function == NULL) {
        return -1;
    }

    task_t *task = malloc(sizeof(task_t));
    if (task == NULL) {
        log_error("작업 메모리 할당 실패");
        return -1;
    }

    task->function = function;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->queue_mutex);

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        free(task);
        return -1;
    }

    // Add task to queue
    if (pool->task_queue_tail == NULL) {
        pool->task_queue_head = task;
        pool->task_queue_tail = task;
    } else {
        pool->task_queue_tail->next = task;
        pool->task_queue_tail = task;
    }

    pool->active_tasks++;
    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}

void thread_pool_wait(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->queue_mutex);
    while (pool->active_tasks > 0 || pool->task_queue_head != NULL) {
        pthread_cond_wait(&pool->queue_empty_cond, &pool->queue_mutex);
    }
    pthread_mutex_unlock(&pool->queue_mutex);
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Wait for all threads to finish
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // Clean up remaining tasks
    pthread_mutex_lock(&pool->queue_mutex);
    task_t *task = pool->task_queue_head;
    while (task != NULL) {
        task_t *next = task->next;
        free(task);
        task = next;
    }
    pthread_mutex_unlock(&pool->queue_mutex);

    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
    pthread_cond_destroy(&pool->queue_empty_cond);
    free(pool->threads);
    free(pool);

    log_info("스레드 풀 종료 완료");
}

static void* thread_pool_worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t*)arg;

    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);

        // Wait for task or shutdown signal
        while (pool->task_queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }

        if (pool->shutdown && pool->task_queue_head == NULL) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }

        // Get task from queue
        task_t *task = pool->task_queue_head;
        if (task != NULL) {
            pool->task_queue_head = task->next;
            if (pool->task_queue_head == NULL) {
                pool->task_queue_tail = NULL;
            }
        }

        pthread_mutex_unlock(&pool->queue_mutex);

        // Execute task
        if (task != NULL) {
            task->function(task->arg);
            free(task);

            pthread_mutex_lock(&pool->queue_mutex);
            pool->active_tasks--;
            if (pool->active_tasks == 0) {
                pthread_cond_broadcast(&pool->queue_empty_cond);
            }
            pthread_mutex_unlock(&pool->queue_mutex);
        }
    }

    return NULL;
}
