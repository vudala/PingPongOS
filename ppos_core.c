#include "ppos.h"
#include "ppos_data.h"

#include <stdio.h>

int id_counter = 0;

void ppos_init()
{
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0) ;
}

int task_create (task_t * task, void (*start_routine)(void *), void * arg)
{
    task_t * task = malloc(sizeof(task_t));
    if (!task) return -1;

    task->id = id_counter++;
    task->status = READY;
    task->prev = NULL;
    task->next = NULL;

    ucontext_t curr;
    getcontext(&curr);
    makecontext(&curr, start_routine, 1, arg);

    return task->id;
}
