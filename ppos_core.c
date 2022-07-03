#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

int Id_Counter = 1;

task_t Main_Task, Dispatcher_Task;
task_t * Current_Task = NULL;

// as filas de task
task_t * Ready_Tasks = NULL;
task_t * Running_Tasks = NULL;

task_t * scheduler()
{
    task_t * task_selected = Ready_Tasks;
    queue_remove((queue_t **) &Ready_Tasks, (queue_t*) task_selected);

    #ifdef DEBUG
    printf ("scheduler: tarefa %d selecionada\n", task_selected->id) ;
    #endif
    return task_selected;
}


void prele(task_t * el) {
    printf("%d ", el->id);
}


void dispatcher(void * arg)
{
    #ifdef DEBUG
    printf ("dispatcher iniciado\n") ;
    #endif
    task_t * next = NULL;
    while(Ready_Tasks || Running_Tasks)
    {
        next = scheduler();
        if (next) {
            queue_append((queue_t **) &Running_Tasks, (queue_t*) next);

            #ifdef DEBUG
            queue_print(NULL, (queue_t*) Ready_Tasks,(void*) prele);
            queue_print(NULL, (queue_t*) Running_Tasks,(void*) prele);
            #endif

            next->status = RUNNING;
            task_switch(next);
            switch (next->status)
            {
            case READY:
                perror("Error while handling tasks.");
                exit(UNEXPECTED_BEHAVIOUR);
                break;
            case RUNNING:
                queue_remove((queue_t **) &Running_Tasks, (queue_t*) next);
                queue_append((queue_t **) &Ready_Tasks, (queue_t*) next);
                next->status = READY;
                break;
            case SUSPENDED:
                // a ser implementado no futuro
                break;
            case DONE:
                queue_remove((queue_t **) &Running_Tasks, (queue_t*) next);
                break;
            default:
                perror("Error while handling tasks.");
                exit(UNEXPECTED_BEHAVIOUR);
                break;
            }
        }
    }
    #ifdef DEBUG
    printf ("dispatcher encerrado\nretornando para a main\n") ;
    #endif
    task_switch(&Main_Task);
}


void create_dispatcher_task()
{
    task_create(&Dispatcher_Task, dispatcher, NULL);
    queue_remove((queue_t **) &Ready_Tasks, (queue_t*) &Dispatcher_Task);
}


void create_main_task()
{
    Main_Task.id = 0;
    Main_Task.context = malloc(sizeof(ucontext_t));
    if (!Main_Task.context) {
        perror("Error while allocating memory for the main context: ");
        exit(UNEXPECTED_BEHAVIOUR);
    }
    getcontext(Main_Task.context);
}


void ppos_init()
{
    create_main_task();
    create_dispatcher_task();

    Current_Task = &Main_Task;

    setvbuf (stdout, 0, _IONBF, 0);
}


int task_create (task_t * task, void (*start_routine)(void *), void * arg)
{
    task->id = Id_Counter++;
    task->prev = NULL;
    task->next = NULL;
    task->preemptable = 0;
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

    task->status = READY;
    queue_append((queue_t **) &Ready_Tasks, (queue_t *) task);

    return task->id;
}


int task_switch (task_t * task)
{
    #ifdef DEBUG
    printf ("task_switch: trocando tarefa %d -> %d\n", task_id(), task->id) ;
    #endif

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
    task_switch(&Dispatcher_Task);
}


int task_id()
{
    return Current_Task->id;
}


void task_yield ()
{
    task_switch(&Dispatcher_Task);
}