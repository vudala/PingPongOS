#include "ppos.h"
#include "ppos_data.h"

#include <stdio.h>
#include <stdlib.h>

int Id_Counter = 1;

task_t Main_Task;
task_t * Current_Task = NULL;

void ppos_init()
{
    // inicializa os valores da main task
    Main_Task.id = 0;
    Main_Task.context = malloc(sizeof(ucontext_t));
    getcontext(Main_Task.context);

    Current_Task = &Main_Task;

    setvbuf (stdout, 0, _IONBF, 0);
}

int task_create (task_t * task, void (*start_routine)(void *), void * arg)
{
    task->id = Id_Counter++;
    task->status = READY;
    task->prev = NULL;
    task->next = NULL;
    task->context = malloc(sizeof(ucontext_t));

    getcontext(task->context);

    void * stack = malloc (STACKSIZE) ;
    if (stack)
    {
        task->context->uc_stack.ss_sp = stack ;
        task->context->uc_stack.ss_size = STACKSIZE ;
        task->context->uc_stack.ss_flags = 0 ;
        task->context->uc_link = 0 ;
    }
    else
    {
        perror ("Error while creating the stack: ") ;
        return TASK_CREATE_FAILURE;
    }

    makecontext(task->context, (void*) start_routine, 1, arg);

    return task->id;
}


int task_switch (task_t * task)
{
    // variavel auxiliar pra nao perder o ponteiro salvo em Current_Task
    ucontext_t * aux = Current_Task->context;
    Current_Task = task;

    return swapcontext(aux, task->context);
}


void task_exit (int exit_code)
{
    task_switch(&Main_Task);
}


int task_id() {
    return Current_Task->id;
}
