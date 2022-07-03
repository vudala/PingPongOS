// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

#define STACKSIZE 64*1024

enum TASK_STATUS {
  READY = 1,
  DONE,
  RUNNING,
  SUSPENDED
};

enum ERROR_CODE {
  TASK_CREATE_FAILURE = -288,
  UNEXPECTED_BEHAVIOUR
};

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;	// ponteiros para usar em filas
  unsigned int id ;				              // identificador da tarefa
  ucontext_t * context ;			    // contexto armazenado da tarefa
  unsigned int status ;			          // pronta, rodando, suspensa  (0, 1, )
  short preemptable ;			      // pode ser preemptada?
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
