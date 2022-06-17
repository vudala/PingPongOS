#include "queue.h"

#include <stdio.h>

void trigger_error(const char * msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

int queue_size(queue_t * queue)
{
    if (queue == NULL)
        return 0;

    int counter = 0;

    queue_t * next = queue;
    do {
        counter += 1;
        next = next->next;
    } while (next != queue);
    

    return counter;
}


void queue_print(char *name, queue_t *queue, void print_elem (void*))
{
    if (queue == NULL)
        return;

    queue_t * next = queue;
    do {
        print_elem(next);
        next = next->next;
    } while (next != queue);

    printf("\n");
}


int queue_append (queue_t ** queue_ptr, queue_t *elem)
{
    if (queue_ptr == NULL)
    {
        trigger_error("queue pointer is NULL");
        return 1;
    }
        
    if (elem == NULL){
        trigger_error("element is NULL");
        return 2;
    }

    // se prev ou next ja apontam pra outro nodo retorna erro
    if (elem->next || elem->prev)
    {
        trigger_error("element belongs to another queue");
        return 3;
    }
        

    queue_t * queue = *queue_ptr;

    if (queue == NULL)
    {
        elem->next = elem;
        elem->prev = elem;
        *queue_ptr = elem;
    }
    else {
        elem->next = queue;
        elem->prev = queue->prev;
        queue->prev->next = elem;
        queue->prev = elem;
    }

    return 0;
}

int queue_remove (queue_t **queue_ptr, queue_t *elem)
{
    if (queue_ptr == NULL)
    {
        trigger_error("queue pointer is NULL");
        return 1;
    }

    queue_t * queue = *queue_ptr;
    if (queue_size(queue) < 1)
    {
        trigger_error("queue is empty");
        return 2;
    }
    
    if (elem == NULL)
    {
        trigger_error("element is NULL");
        return 3;
    }

    queue_t * next = queue;
    do {
        if (next == elem)
        {
            if (queue_size(queue) == 1)
                *queue_ptr = NULL;
            else
            {
                elem->prev->next = elem->next;
                elem->next->prev = elem->prev;

                if (elem == queue)
                    *queue_ptr = queue->next;
            }
            elem->prev = NULL;
            elem->next = NULL;

            return 0;
        }
        next = next->next;
    } while (next != queue);

    trigger_error("element does not belong to the queue");
    return 4;
}