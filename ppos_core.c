#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

// Gerenciamento de tasks
int Id_Counter = 0;
task_t Main_Task, Dispatcher_Task;
task_t * Current_Task = NULL;

// Fila de tasks
unsigned int Tasks_Counter = 0;
task_t * Ready_Tasks = NULL;
task_t * Sleeping_Tasks = NULL;

// Relógio do sistema
unsigned int Current_Time;

// Preempção por tempo
int Tick_Counter = QUANTUM;

// Kernel Big Lock
unsigned short Locked = 1;
void lock() { Locked = 1; }     // impede que a preempcao seja efetuada quando lock esta up
void unlock() { Locked = 0; }   // desativa a lock

unsigned int systime()
{
    return Current_Time;
}


void task_destroy(task_t * task)
{
    if (task && task->context.uc_stack.ss_sp)
        free(task->context.uc_stack.ss_sp);
}


task_t * fcfs_scheduler()
{
    task_t * task_selected = Ready_Tasks;
    Ready_Tasks = task_selected->next;

    // move a task que acabou de ser selecionada pro fim da fila
    queue_remove((queue_t **) &Ready_Tasks, (queue_t *) task_selected);
    queue_append((queue_t **) &Ready_Tasks, (queue_t *) task_selected);

    return task_selected;
}


task_t * prio_scheduler()
{
    task_t * task = Ready_Tasks;
    task_t * oldest = Ready_Tasks;

    if (task) {
        do {
            task->dinamic_prio -= AGING_ALPHA;
            if (oldest->dinamic_prio > task->dinamic_prio)
                oldest = task;
            task = task->next;
        } while (task != Ready_Tasks);
        
        oldest->dinamic_prio = oldest->static_prio;
    }

    return oldest;
}


task_t * (*scheduler)() = prio_scheduler;

void update_sleeping()
{
    lock();

    task_t * task = Sleeping_Tasks;
    task_t * next = NULL;
    if (task) {
        do {
            next = task->next;
            if (task->waking_time <= systime())
                task_resume(task, &Sleeping_Tasks);
            task = next;
        } while (task != Sleeping_Tasks && Sleeping_Tasks);
    }
}


void dispatcher(void * arg)
{
    lock();
    unsigned int last_time = systime();
    #ifdef DEBUG
    printf ("dispatcher iniciado\n") ;
    #endif

    task_t * next = NULL;
    while(Tasks_Counter > 1) {
        update_sleeping();
        next = scheduler();

        #ifdef DEBUG
        printf ("scheduler: tarefa %d selecionada\n", next->id) ;
        #endif

        if (next) {
            next->status = RUNNING;
            Tick_Counter = QUANTUM;
            unsigned int start_time = systime();
            Dispatcher_Task.lifetime += systime() - last_time;
            task_switch(next);
            lock();
            next->lifetime += systime() - start_time;
            last_time = systime();

            switch (next->status)
            {
            case RUNNING:
                next->status = READY;
                break;
            case DONE:
                queue_remove((queue_t **) &Ready_Tasks, (queue_t *) next);
                task_destroy(next);
                break;
            case SUSPENDED:
                break;
            default:
                exit(UNEXPECTED_BEHAVIOUR);
                break;
            }
        }
    }
    #ifdef DEBUG
    printf ("dispatcher encerrado\nretornando para a main\n") ;
    #endif

    task_exit(0);
}


void create_dispatcher_task()
{
    task_create(&Dispatcher_Task, dispatcher, NULL);
}


void create_main_task()
{
    task_create(&Main_Task, NULL, NULL);
}


void tick_handler(int signum)
{
    Current_Time += 1;
    if (Locked) return;
    
    if (Tick_Counter == 0)
        task_yield();
    else
        Tick_Counter -= 1;
}


// define qual acao deve ser tomada em cada tick de clock
void set_tick_handler()
{
    struct sigaction action;

    action.sa_handler = tick_handler;
    sigemptyset(&action.sa_mask) ;
    action.sa_flags = 0 ;

    if (sigaction(SIGALRM, &action, 0) < 0)
        exit(UNEXPECTED_BEHAVIOUR);
}


// define o comportamento do clock
void set_timer()
{
    struct itimerval timer;

    // quanto tempo para disparar o primeiro tick
    timer.it_value.tv_usec = 1;
    timer.it_value.tv_sec  = 0;
    // intervalo entre os ticks
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec  = 0;

    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
        exit(UNEXPECTED_BEHAVIOUR);
}


void ppos_init()
{
    lock();

    setvbuf (stdout, 0, _IONBF, 0);

    create_main_task();
    create_dispatcher_task();

    set_tick_handler();
    set_timer();

    Current_Time = 0;
    Current_Task = &Main_Task;

    task_yield();
}


int task_create(task_t * task, void (*start_routine)(void *), void * arg)
{
    lock();

    if (!task)
        return TASK_CREATE_FAILURE;

    task->id = Id_Counter++;
    task->prev = NULL;
    task->next = NULL;
    task->static_prio = 0;
    task->dinamic_prio = 0;
    task->birth_time = systime();
    task->lifetime = 0;
    task->awaiting_tasks = NULL;

    task->status = NEW;

    // cria uma stack e seta o contexto para a funcao do parametro
    if (task != &Main_Task) {
        if (start_routine == NULL)
            return TASK_CREATE_FAILURE;

        getcontext(&(task->context));

        void * stack = malloc (STACKSIZE) ;
        if (stack) {
            task->context.uc_stack.ss_sp = stack ;
            task->context.uc_stack.ss_size = STACKSIZE ;
            task->context.uc_stack.ss_flags = 0 ;
            task->context.uc_link = 0 ;
        }
        else
            return TASK_CREATE_FAILURE;

        makecontext(&(task->context), (void*) start_routine, 1, arg);
    }
    // se for a main continua com o contexto atual
    else
        task->context.uc_stack.ss_sp = NULL;
        
    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif

    // adiciona na fila de prontas
    task->status = READY;
    if (task != &Dispatcher_Task)
        queue_append((queue_t **) &Ready_Tasks, (queue_t *) task);

    Tasks_Counter += 1;

    unlock();
    return task->id;
}


int task_switch(task_t * task)
{
    if (!task)
        return -1;

    #ifdef DEBUG
    printf ("task_switch: trocando tarefa %d -> %d\n", task_id(), task->id) ;
    #endif

    task->activations += 1;

    ucontext_t * aux = &(Current_Task->context);
    Current_Task = task;

    if (task != &Dispatcher_Task) unlock();
    return swapcontext(aux, &(task->context));
}


void task_resume(task_t * task, task_t ** queue)
{
    if (task) {
        if (queue)
            queue_remove((queue_t **) queue, (queue_t*) task);
        task->status = READY;
        queue_append((queue_t **) &Ready_Tasks, (queue_t*) task);
    }
}


void task_exit(int exit_code)
{
    lock();

    // imprime as medicoes de tempo da task
    printf(
        "Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
        Current_Task->id,
        systime() - Current_Task->birth_time,
        Current_Task->lifetime,
        Current_Task->activations
    );

    #ifdef DEBUG
    printf ("task_exit: encerrando tarefa %d\n", task_id()) ;
    #endif

    // sinaliza pro dispatcher que a task ja pode ser removida da fila
    Current_Task->status = DONE;

    Current_Task->exit_code = exit_code;

    // enquanto houver tasks, acorda a primeira da fila e a remove
    while(Current_Task->awaiting_tasks)
        task_resume(Current_Task->awaiting_tasks, &(Current_Task->awaiting_tasks));

    Tasks_Counter -= 1;

    task_switch(&Dispatcher_Task);
}


int task_id()
{
    return Current_Task->id;
}


void task_yield()
{
    lock();
    task_switch(&Dispatcher_Task);
}


void task_setprio(task_t * task, int prio)
{
    lock();

    if (prio < -20 || prio > 20) {unlock(); return; }

    if (task) {
        task->static_prio = prio;
        task->dinamic_prio = prio;
    }
    else {
        Current_Task->static_prio = prio;
        Current_Task->dinamic_prio = prio;
    }

    unlock();
}


int task_getprio(task_t * task)
{
    if (task)
        return task->static_prio;
    else
        return Current_Task->static_prio;
}


void task_suspend(task_t ** queue)
{
    queue_remove((queue_t **) &Ready_Tasks, (queue_t*) Current_Task);
    Current_Task->status = SUSPENDED;
    if (queue)
        queue_append((queue_t **) queue, (queue_t*) Current_Task);

    task_yield();
}


int task_join(task_t * task)
{
    lock();

    // impede join em tarefa inexistente
    if (!task) { unlock(); return -1; }

    // impede deadlock
    if (task == Current_Task) { unlock(); return -1; }
    
    // se ja terminou apenas retorna o exit code
    if (task->status == DONE) { unlock(); return task->exit_code; }

    task_suspend(&(task->awaiting_tasks));

    unlock();

    return task->exit_code;
}


void task_sleep(int t)
{
    lock();

    Current_Task->waking_time = systime( ) + t;
    task_suspend(&Sleeping_Tasks);
}


item_t * create_item(void * msg, int size)
{
    item_t * it = malloc(sizeof(item_t));
    if (!it) return NULL;

    it->prev = NULL;
    it->next = NULL;
    it->value = malloc(size);
    if (!it->value) return NULL;

    memcpy(it->value, msg, size);

    return it;
}


int mqueue_create (mqueue_t *queue, int max_msgs, int msg_size)
{
    if (!queue) return -1;

    sem_create(&queue->sem_item, 0);
    sem_create(&queue->sem_buffer, 1);
    sem_create(&queue->sem_vaga, max_msgs); 
    queue->item_size = msg_size;
    queue->items = NULL;
    queue->destroyed = 0;

    return 0;
}


int mqueue_send (mqueue_t *queue, void * msg)
{
    if (!queue || !msg || queue->destroyed) return -1;

    sem_down(&queue->sem_vaga);
    sem_down(&queue->sem_buffer);

    if (queue->destroyed) return -1;
    
    item_t * it = create_item(msg, queue->item_size);
    queue_append((queue_t**) &queue->items, (queue_t*) it);

    sem_up(&queue->sem_buffer);
    sem_up(&queue->sem_item);

    return 0;
};


// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg)
{
    if (!queue || !msg || queue->destroyed) return -1;

    sem_down(&queue->sem_item);
    sem_down(&queue->sem_buffer);
    
    if (queue->destroyed) return -1;

    item_t * item = queue->items;
    queue_remove((queue_t**) &queue->items, (queue_t*) item);
    memcpy(msg, item->value, queue->item_size);
    free(item->value);
    free(item);

    sem_up(&queue->sem_buffer);
    sem_up(&queue->sem_vaga);
    
    return 0;
};


// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue)
{
    if (!queue) return -1;
    if (queue->destroyed) return 0;

    lock();

    while(queue->items) {
        item_t * it = queue->items;
        queue_remove((queue_t**) &queue->items, (queue_t*) it);
        free(it->value);
        free(it);
    }

    sem_destroy(&queue->sem_vaga);
    sem_destroy(&queue->sem_item);
    sem_destroy(&queue->sem_buffer);

    queue->destroyed = 1;

    unlock();

    return 0;
};


// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue)
{
    if (!queue) return 0;
    return queue_size((queue_t*) queue->items);
};