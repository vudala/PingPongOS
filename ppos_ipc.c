#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdlib.h>
#include <stdio.h>

extern task_t * Current_Task;
extern task_t * Ready_Tasks;

int sem_create (semaphore_t *sem, int value)
{
    if (!sem) return -1;

    sem->counter = value;
    sem->queue = NULL;
    sem->lock = 0;

    return 0;
}


void enter_cs (int * lock)
{
    while (__sync_fetch_and_and (lock, 1));   // busy waiting
}


void leave_cs (int * lock)
{
    (*lock) = 0;
}

int sem_down (semaphore_t *sem)
{
    if (!sem) return -1;

    enter_cs(&sem->lock);
    sem->counter -= 1;
    if (sem->counter < 0) {
        leave_cs(&sem->lock);
        task_suspend(&sem->queue);
    }

    return 0;
}


int sem_up (semaphore_t *sem)
{
    if (!sem) return -1;

    enter_cs(&sem->lock);
    sem->counter += 1;
    if (sem->counter <= 0) {
        leave_cs(&sem->lock);
        task_t * t = sem->queue;
        task_resume(t, &sem->queue);
        return t->awaking_code;
    }
    return 0;
}


int sem_destroy (semaphore_t *sem)
{
    if (!sem) return -1;

    while(sem->queue) {
        sem->queue->awaking_code = -1;
        task_resume(sem->queue, &sem->queue);
    }
        
    return 0;
}
