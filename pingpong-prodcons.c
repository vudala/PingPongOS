#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

typedef struct message_t {
    struct message_t *prev, *next;
    int value;
} message_t;


message_t * buffer = NULL;
semaphore_t s_vaga, s_buffer, s_item;


message_t * c_item(int v)
{
    message_t * it = malloc(sizeof(message_t));
    if (!it) return NULL;
    
    it->value = v;
    it->prev = NULL;
    it->next = NULL;

    return it;
}


int prod_id = 1;
void produtor() {
    int id = prod_id++;
    for(;;) {
        task_sleep(1000);

        sem_down (&s_vaga);
        sem_down (&s_buffer);
        
        // coloca item no buffer
        message_t * it = c_item(rand() % 100);
        queue_append((queue_t**) &buffer, (queue_t*) it);

        printf("p%d produziu %d\n", id, it->value);

        sem_up (&s_buffer);
        sem_up (&s_item);
    }
    task_exit(0);
}


int cons_id = 1;
void consumidor() {
    int id = cons_id++;
    for(;;) {
        sem_down (&s_item);
        sem_down (&s_buffer);

        // retira item do buffer
        message_t * it = buffer;
        queue_remove((queue_t**) &buffer, (queue_t*) it);

        sem_up (&s_buffer);
        sem_up (&s_vaga);

        // print item
        printf("                     c%d consumiu %d\n", id, it->value);
        task_sleep(1000);
    }
    task_exit(0);
}


task_t produtor1, produtor2, produtor3;
task_t consumidor1, consumidor2;


int main() {
    srand(time(NULL));

    printf("main: inicio\n");

    ppos_init();

    sem_create(&s_vaga, 5);
    sem_create(&s_buffer, 1);
    sem_create(&s_item, 0);

    task_create(&produtor1, produtor, NULL);
    task_create(&produtor2, produtor, NULL);
    task_create(&produtor3, produtor, NULL);
    
    task_create(&consumidor1, consumidor, NULL);
    task_create(&consumidor2, consumidor, NULL);

    task_exit(0);

    exit(0);
    return 0;
}