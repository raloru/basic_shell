/* 
~~~~~~~~~SoW Shell~~~~~~~~~
Ra�l L�pez Rueda - 79044674E
*/


#ifndef _JOB_CONTROL_H
#define _JOB_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

// ----------- ENUMERATIONS ---------------------------------------------
enum status { SUSPENDED, SIGNALED, EXITED, CONTINUED};
enum job_state { FOREGROUND, BACKGROUND, STOPPED };
static char* status_strings[] = { "Suspended", "Signaled", "Exited", "Continued"};
static char* state_strings[] = { "Foreground", "Background", "Stopped" };

// ----------- JOB TYPE FOR JOB LIST ------------------------------------
typedef struct job_
{
	pid_t pgid; /* group id = process lider id */
	char * command; /* program name */
	enum job_state state;
	struct job_ *next; /* next job in the list */
} job;



// -----------------------------------------------------------------------
//      PUBLIC FUNCTIONS
// -----------------------------------------------------------------------

void get_command(char inputBuffer[], int size, char *args[], int *background, int*redir, char**in, char**out,char** pipe, char *args2[]);

job * new_job(pid_t pid, const char * command, enum job_state state);

void add_job (job * list, job * item);

int delete_job(job * list, job * item);

job * get_item_bypid  (job * list, pid_t pid);

job * get_item_bypos( job * list, int n);

enum status analyze_status(int status, int *info);

// -----------------------------------------------------------------------
//      PRIVATE FUNCTIONS: BETTER USED THROUGH MACROS BELOW
// -----------------------------------------------------------------------

void print_item(job * item);

void print_list(job * list, void (*print)(job *));

void terminal_signals(void (*func) (int));

void block_signal(int signal, int block);

// -----------------------------------------------------------------------
//      PUBLIC MACROS
// -----------------------------------------------------------------------

#define list_size(list) 	 list->pgid   // number of jobs in the list
#define empty_list(list) 	 !(list->pgid)  // returns 1 (true) if the list is empty

#define new_list(name) 			 new_job(0,name,FOREGROUND)  // name must be const char *

#define print_job_list(list) 	 print_list(list, print_item)

#define restore_terminal_signals()   terminal_signals(SIG_DFL)
#define ignore_terminal_signals() terminal_signals(SIG_IGN)

#define set_terminal(pid)        tcsetpgrp (STDIN_FILENO,pid)
#define new_process_group(pid)   setpgid (pid, pid)

#define block_SIGCHLD()   	 block_signal(SIGCHLD, 1)
#define unblock_SIGCHLD() 	 block_signal(SIGCHLD, 0)

// macro for debugging----------------------------------------------------
// to debug integer i, use:    debug(i,%d);
// it will print out:  current line number, function name and file name, and also variable name, value and type
#define debug(x,fmt) fprintf(stderr,"\"%s\":%u:%s(): --> %s= " #fmt " (%s)\n", __FILE__, __LINE__, __FUNCTION__, #x, x, #fmt)

// -----------------------------------------------------------------------
#endif

