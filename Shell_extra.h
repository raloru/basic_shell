/* 
~~~~~~~~~SoW Shell~~~~~~~~~
Raúl López Rueda - 79044674E
*/


#include "job_control.h"
#include <pwd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

// ----------- TIMEOUT Structure------------------------------------

typedef struct {
  pid_t   pgid;
  int time;
} elem;


void print_children(int pid, int proc[500],int num);

void childrens();

char getchShell();

void manejadorAlarma(int sig);

void traslade(char *args[]);

void print_analyzed_status(int status, int info);

void copiaFichero(FILE * f1, char * nombre);

static void* killer(void *arg);

void timeout_pthread_sleep(int pid, int time);

