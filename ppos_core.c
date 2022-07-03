#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

int Id_Counter = 1;

task_t Main_Task, Dispatcher_Task;
task_t * Current_Task = NULL;
task_t * Tasks_Queue = NULL;

task_t * scheduler()
{
    task_t * task_selected = Tasks_Queue;

    while (task_selected->status == RUNNING)
        task_selected = task_selected->next;

    return task_selected;
}

void prele(task_t * el) {
    printf("%d ", el->id);
}

void dispatcher(void * arg)
{
    task_t * next = NULL;
    while(queue_size((queue_t*) Tasks_Queue) > 0)
    {
        next = scheduler();
        if (next) {
            task_switch(next);
            switch (next->status)
            {
            case READY:
                // faz nada
                break;
            case RUNNING:
                // isso não deve acontecer
                break;
            case SUSPENDED:
                queue_remove((queue_t **) &Tasks_Queue, (queue_t*) next);
                queue_append((queue_t **) &Tasks_Queue, (queue_t*) next);
                break;
            case DONE:
                queue_remove((queue_t **) &Tasks_Queue, (queue_t*) next);
                break;
            default:

                break;
            }
        }
    }
    task_exit(0);
}


void ppos_init()
{
    // inicializa os valores da main task
    Main_Task.id = 0;
    Main_Task.context = malloc(sizeof(ucontext_t));
    if (!Main_Task.context) {
        perror("Error while allocating memory for the main context: ");
        exit(UNEXPECTED_BEHAVIOUR);
    }
    getcontext(Main_Task.context);

    task_create(&Dispatcher_Task, dispatcher, NULL);

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

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif

    queue_append((queue_t **) &Tasks_Queue, (queue_t *) task);

    return task->id;
}


int task_switch (task_t * task)
{
    #ifdef DEBUG
    printf ("task_switch: trocando tarefa %d -> %d\n", task_id(), task->id) ;
    #endif

    Current_Task->status = SUSPENDED;
    task->status = RUNNING;

    // variavel auxiliar pra nao perder o ponteiro salvo em Current_Task
    ucontext_t * aux = Current_Task->context;
    Current_Task = task;

    return swapcontext(aux, task->context);
}


void task_exit (int exit_code)
{
    #ifdef DEBUG
    printf ("task_exit: encerrando tarefa %d\n", task_id()) ;
    #endif
    Current_Task->status = DONE;
    task_switch(&Main_Task);
}


int task_id()
{
    return Current_Task->id;
}


void task_yield ()
{
    task_switch(&Dispatcher_Task);
}