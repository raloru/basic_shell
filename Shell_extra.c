/* 
~~~~~~~~~SoW Shell~~~~~~~~~
Raúl López Rueda - 79044674E
*/

#include "Shell_extra.h"

static void* killer(void *arg) {
  elem *payload = arg;

  sleep(payload->time);
  killpg(payload->pgid,SIGTERM);
}

void timeout_pthread_sleep(int pid, int time) {
	pthread_t thid;
	elem *theOne;
	int pgid = pid;
	int when = time;
	int crea;

	theOne = (elem *)malloc(sizeof(elem));
	theOne->pgid = pgid;
	theOne->time = when;

	if (0 == pthread_create(&thid,NULL,killer,(void *)theOne))
		pthread_detach(thid);
	else
		fprintf(stderr,"pthread_create: error\n");
    
   }


void print_children(int pid, int procesos[500], int num) {
    int hijos = 0, i;
    printf("%-5d\t", pid);

    char filename[1000];
    sprintf(filename, "/proc/%d/stat", pid);
    FILE *f = fopen(filename, "r");

    int unused;
    long unused2;
    long int unused3, thr;

    char comm[1000];
    char state;
    int ppid;

    for (i = 0; i < num; i++) {
        sprintf(filename, "/proc/%d/stat", procesos[i]);
        FILE *f = fopen(filename, "r");
        fscanf(f, "%d %s %c %d ", &unused, comm, &state, &ppid);
        if(ppid == pid)
            hijos++;
        fclose(f);
    }
    

    fscanf(f, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld", &unused, comm, &state, &ppid, &unused, &unused, &unused, &unused,&unused,&unused2,&unused2,&unused2,&unused2,&unused2,&unused2,&unused3,&unused3,&unused3,&unused3,&thr);
    printf("%-5d\t", ppid);
    printf("%-20s\t", comm);
    printf("%-5d\t", hijos);
    printf("%-3ld\n", thr);
    fclose(f);
}


void childrens() {
    DIR *mydir;
    struct dirent *myfile;
    int esproc = 1, i = 0;
    int procesos[500];
    int num = 0;
    
    mydir = opendir("/proc");

    char buf[512];
    while ((myfile = readdir(mydir)) != NULL) {
        esproc = 1;
        i = 0;
        while ((myfile->d_name[i] != '\0')&&(esproc)) {
            if (!((myfile->d_name[i] > 47)&&(myfile->d_name[i] < 58))) {
                esproc = 0;
            }
            i++;
        }

        if (esproc) {           
            procesos[num] = atoi(myfile->d_name);
            num++;
        }      
    }
    printf("-----------------------Lista de hijos-----------------------\n");
    printf("PID\tPPID\tCOMANDO\t\t\tHIJOS\tTHREADS\n");
    for(i = 0; i < num; i++) {
        print_children(procesos[i], procesos, num);
    }
    closedir(mydir);
}

char getchShell() {
    int shell_terminal = STDIN_FILENO;
    struct termios conf;
    struct termios conf_new;
    char c;

    tcgetattr(shell_terminal, &conf);
    conf_new = conf;

    conf_new.c_lflag &= (~(ICANON | ECHO));
    conf_new.c_cc[VTIME] = 0;
    conf_new.c_cc[VMIN] = 1;

    tcsetattr(shell_terminal, TCSANOW, &conf_new);

    c = getc(stdin);

    tcsetattr(shell_terminal, TCSANOW, &conf);
    return c;
}

void traslade(char *args[]) {
    int i = 0;
    while (args[i + 2] != NULL) {
        strcpy(args[i], args[i + 2]);
        i++;
    }
    args[i] = NULL;
    args[i + 1] = NULL;
}

void print_analyzed_status(int status, int info) {
	
	if (status == EXITED) {
		printf("---%s, Info: %d.---\n", status_strings[EXITED], info);
	} else if (status == SUSPENDED){
		printf("---%s, Info: %d.---\n", status_strings[SUSPENDED] , info);
	} else if (status == SIGNALED) {
		printf("---%s, Info: %d.---\n", status_strings[SIGNALED] , info);
	}

}

void copiaFichero(FILE * f1, char * nombre) {
    FILE *f2 = fopen(nombre, "r");
    char * aux;
    fflush(stdout);
    while (fgets(aux, 100, f2)) {
        fputs(aux, f1);
        fflush(stdout);
    }
    fclose(f2);
}
