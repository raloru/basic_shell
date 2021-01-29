/* 
~~~~~~~~~SoW Shell~~~~~~~~~
Raúl López Rueda - 79044674E
*/


#include "Shell_extra.h"
#include "job_control.h"


#define MAX_LINE 256

job * jobs;
job * history;

int pid_alarm;
int AlarmActive = 0;
int AlarmOut = 0;

void singchld_handler(int sing) {
    int n;
    for (n = list_size(jobs); n >= 1; n--) {
        job *aux = get_item_bypos(jobs, n);
        if (aux->state != FOREGROUND) {
            int status;
            pid_t wpid = waitpid(aux->pgid, &status, WCONTINUED | WNOHANG | WUNTRACED);

            if (wpid == aux->pgid) {
                int info;
                enum status st = analyze_status(status, &info);
                printf("---Background pid: %d, command: %s, %s, info: %d.---\n", wpid, aux->command, status_strings[st], info);
                if (st == SUSPENDED) {
                    printf("---Job parado.---\n");
                    aux->state = STOPPED;
                } else if (st == SIGNALED) {
                    printf("---Job pasado a background.---\n");
                    aux->state = BACKGROUND;
                } else if (st == EXITED) {
                    printf("---Job finalizado.---\n");
                    aux->state = STOPPED;
                    block_SIGCHLD();
                    delete_job(jobs, aux);
                    unblock_SIGCHLD();
                } else {
                    printf("---Job pasado a background.---\n");
                    aux->state = BACKGROUND;
                }

            }
        }
    }

    fflush(stdout);

}

// -------------------------------------------------------------------- ---
//                            MAIN          
// -----------------------------------------------------------------------

int main(void) {
    char inputBuffer[MAX_LINE];
    int background;
    char *args[MAX_LINE / 2];
    char *args2[MAX_LINE / 2];
    int pid_fork, pid_wait, mypid, pidjob;
    int status;
    enum status status_res;
    int info;
    job* aux;
    job* auxil;
    char* str;
    int i;
    int redir;
    char * in;
    char * out;
    char * pipe;
    FILE *infile;
    FILE *outfile;
    int fnum1, fnum2;
    char ruta[1024];

    ignore_terminal_signals();

    jobs = new_list("Jobs List");
    history = new_list("History");

    signal(SIGCHLD, singchld_handler);
    printf("\n\t\t~~~~~~BIENVENIDO A SoW SHELL~~~~~~\n\n");
    while (1) {
init:
        unblock_SIGCHLD();
        getcwd(ruta, sizeof (ruta));
        printf("%s$ ", ruta);
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background, &redir, &in, &out, &pipe, args2);

begin:

        if (args[0] == NULL) continue;

        if (strcmp(args[0], "history") == 0 || strcmp(args[0], "historial") == 0) {
            if (args[1] == NULL) {
                print_job_list(history);
            } else {
                job * aux = get_item_bypos(history, atoi(args[1]));

                if (aux != NULL) {

                    args[1] = NULL;
                    i = 0;
                    str = strtok(aux->command, " ");
                    while (str != NULL) {
                        args[i] = str;
                        str = strtok(NULL, " ");
                        i++;
                    }

                    background = (aux->state == BACKGROUND);

                    goto execute;
                } else {
                    printf("---Entrada no encontrada en el historial.---\n");
                }
            }
            continue;
        }

        if ((strcmp(args[0], "children") == 0) && (args[1] == NULL)) {
            childrens();
            continue;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] != NULL) {
                chdir(args[1]);
            } else {
                chdir("/home");
            }
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            printf("\n\n\t\t~~~~~~Â¡HASTA PRONTO!~~~~~~\n\n");
            exit(0);
        }

        if (strcmp(args[0], "jobs") == 0) {

            if (list_size(jobs) == 0) {
                printf("----Lista de jobs vacia.----\n");
                fflush(stdout);

            } else {
                print_job_list(jobs);


            }
            continue;
        }

        if (strcmp(args[0], "fg") == 0) {
            job * item;


            if (list_size(jobs) == 0) {
                printf("---Lista de jobs vacia.---\n");
            } else {
                if (args[1] == NULL) {
                    item = get_item_bypos(jobs, list_size(jobs));
                } else if (atoi(args[1]) > list_size(jobs)) {
                    printf("---Error: job no encontrado.---\n");
                    continue;
                } else {
                    item = get_item_bypos(jobs, atoi(args[1]));
                }

                pid_t pidjo = item->pgid;
                kill(-pidjo, SIGCONT);
                item->state = FOREGROUND;
                printf("---Ejecutando proceso.---\n");
                set_terminal(pidjo);

                waitpid(pidjo, &status, WUNTRACED);

                enum status sts = analyze_status(status, &info);




                block_SIGCHLD();

                if (sts == SIGNALED || sts == EXITED) {
                    printf("---Job eliminado.---\n");
                    delete_job(jobs, item);
                } else if (sts == SUSPENDED) {
                    printf("---Job parado.---\n");
                    item->state = STOPPED;
                }
                unblock_SIGCHLD();
                set_terminal(getpid());
                fflush(stdout);
                continue;
            }
            continue;
        }

        if (strcmp(args[0], "bg") == 0) {
            job * item;
            if (list_size(jobs) == 0) {
                printf("---Lista de jobs vacia.---\n");
            } else {
                if (args[1] == NULL) {
                    item = get_item_bypos(jobs, list_size(jobs));
                } else if (atoi(args[1]) > list_size(jobs)) {
                    printf("---Error: job no encontrado.---\n");
                    continue;
                } else {
                    item = get_item_bypos(jobs, atoi(args[1]));
                }
                pid_t pidjo;
                pidjo = item->pgid;
                kill(-pidjo, SIGCONT);
                enum status sts = analyze_status(status, &info);


                if (sts == SUSPENDED) {
                    printf("---Job movido a background.---\n");
                    item->state = BACKGROUND;
                }
                set_terminal(getpid());
                continue;
            }
            continue;
        }

        if (strcmp(args[0], "timeout") == 0) {

            if (args[1] == NULL) {
                printf("---Introduzca un tiempo valido.---\n");
                continue;
            } else {

                pid_fork = fork();
                if (pid_fork == -1) {
                    printf("---Error en el fork.---\n");
                    exit(EXIT_FAILURE);
                } else if (pid_fork) {
                    // Padre = shell
                    struct termios conf;
                    int shell_terminal;
                    shell_terminal = STDIN_FILENO;
                    tcgetattr(shell_terminal, &conf);
                    new_process_group(pid_fork);
                    if (background) {
                        printf("---Background pid: %d, comando: %s.---\n", pid_fork, args[0]);
                        aux = new_job(pid_fork, args[0], BACKGROUND);
                        block_SIGCHLD();
                        add_job(jobs, aux);
                        unblock_SIGCHLD();
                    } else {
                        set_terminal(pid_fork);
                        pid_wait = waitpid(pid_fork, &status, WUNTRACED);
                        set_terminal(getpid());

                        int status2;
                        status2 = analyze_status(status, &info);
                        printf("---Foreground pid: %d, comando: %s.---\n", pid_fork, args[0]);
                        print_analyzed_status(status2, info);

                        if (status2 == SUSPENDED) {
                            aux = new_job(pid_fork, args[0], STOPPED);
                            block_SIGCHLD();
                            add_job(jobs, aux);
                            unblock_SIGCHLD();
                        }

                    }
                    tcsetattr(shell_terminal, TCSANOW, &conf);
                    continue;
                } else {
                    new_process_group(getpid());
                    if (!background) {
                        set_terminal(getpid());
                    }

                    restore_terminal_signals();

                    execvp(args[0], args);
                    printf("---Error: comando %s no encontrado.---", args[0]);
                    exit(EXIT_FAILURE);

                }
                timeout_pthread_sleep(pid_fork, atoi(args[1]));
                continue;
            }
        }


execute:

        pid_fork = fork();
        if (!AlarmActive) {
            pid_alarm = pid_fork;
            AlarmActive = 1;
        }
        switch (pid_fork) {
            case -1:
                printf("---Error en el fork.---\n");
                fflush(stdout);
                exit(-1);
                break;

            case 0:
                if (redir > 0) {
                    if (redir == 1) {
                        int existe = 0;
                        if (NULL != (outfile = fopen(out, "r"))) {
                            existe = 1;
                            mkdir("/temp", 0777);
                            FILE * f = fopen("/temp/a.temp", "w");
                            copiaFichero(f, out);
                            fclose(outfile);
                            fclose(f);
                        }
                        if (NULL == (outfile = fopen(out, "w"))) {
                            printf("---Error abriendo el fichero %s.---\n", out);
                            return (-1);
                        }
                        fnum1 = fileno(outfile);
                        fnum2 = fileno(stdout);
                        dup2(fnum1, fnum2);
                        restore_terminal_signals();
                        execvp(args[0], args);
                        if (existe) {
                            remove(out);
                            outfile = fopen(out, "w");
                            copiaFichero(outfile, "/temp/a.temp");
                            remove("/temp/a.temp");
                            rmdir("/temp");
                            fflush(stdout);
                            fclose(outfile);
                            existe = 0;
                        } else {
                            fflush(stdout);
                            remove(out);
                        }
                        printf("---Error: comando no encontrado.---\n");
                        exit(-1);

                    } else if (redir == 2) {

                        if (NULL == (infile = fopen(in, "r"))) {
                            printf("---Error abriendo el fichero %s.---\n", out);
                            exit(-1);
                        }

                        fnum1 = fileno(infile);
                        fnum2 = fileno(stdin);
                        dup2(fnum1, fnum2);
                        restore_terminal_signals();
                        execvp(args[0], args);
                        fclose(infile);
                        exit(-1);
                    } else if (redir == 3) {

                        pid_fork = fork();
                        if (!AlarmActive) {
                            pid_alarm = pid_fork;
                            AlarmActive = 1;
                        }
                        switch (pid_fork) {
                            case -1:
                                printf("---Error en el fork.---\n");
                                fflush(stdout);
                                exit(-1);
                                break;

                            case 0:
                                if (redir > 0) {
                                    if (redir == 1) {
                                        int existe = 0;
                                        if (NULL != (outfile = fopen(out, "r"))) {
                                            existe = 1;
                                            mkdir("/temp", 0777);
                                            FILE * f = fopen("/temp/a.temp", "w");
                                            copiaFichero(f, out);
                                            fclose(outfile);
                                            fclose(f);
                                        }
                                        if (NULL == (outfile = fopen(out, "w"))) {
                                            printf("---Error abriendo el fichero %s.---\n", out);
                                            return (-1);
                                        }
                                        fnum1 = fileno(outfile);
                                        fnum2 = fileno(stdout);
                                        dup2(fnum1, fnum2);
                                        restore_terminal_signals();
                                        execvp(args[0], args);
                                        if (existe) {
                                            remove(out);
                                            outfile = fopen(out, "w");
                                            copiaFichero(outfile, "/temp/a.temp");
                                            remove("/temp/a.temp");
                                            rmdir("/temp");
                                            fflush(stdout);
                                            fclose(outfile);
                                            existe = 0;
                                        } else {
                                            fflush(stdout);
                                            remove(out);
                                        }
                                        printf("---Error: comando no encontrado.---\n");
                                        exit(-1);

                                    } else if (redir == 2) {

                                        if (NULL == (infile = fopen(in, "r"))) {
                                            printf("---Error abriendo el fichero %s.---\n", out);
                                            exit(-1);
                                        }

                                        fnum1 = fileno(infile);
                                        fnum2 = fileno(stdin);
                                        dup2(fnum1, fnum2);
                                        restore_terminal_signals();
                                        execvp(args[0], args);
                                        fclose(infile);
                                        exit(-1);
                                    } else if (redir == 3) {

                                        if (NULL == (outfile = fopen("pipe", "w"))) {
                                            printf("---Error abriendo el fichero %s.---\n", "pipe");
                                            return (-1);
                                        }
                                        fnum1 = fileno(outfile);
                                        fnum2 = fileno(stdout);
                                        dup2(fnum1, fnum2);
                                        restore_terminal_signals();
                                        execvp(args[0], args);
                                        printf("---Error: comando no encontrado.---\n");
                                        exit(-1);

                                    }
                                } else {
                                    restore_terminal_signals();
                                    execvp(args [0], args);
                                    printf("---Error: comando %s no encontrado.---", args[0]);
                                    exit(-1);

                                }
                                break;

                            default:
                                if (redir == 3) {
                                    new_process_group(pid_fork);
                                    set_terminal(pid_fork);

                                    pid_wait = waitpid(pid_fork, &status, WUNTRACED);

                                    set_terminal(getpid());

                                    status_res = analyze_status(status, &info);

                                    add_job(history, new_job(pid_fork, args[0], FOREGROUND));
                                    if (NULL == (infile = fopen("pipe", "r"))) {
                                        printf("---Error abriendo el fichero %s.---\n", "pipe");
                                        exit(-1);
                                    }
                                    fnum1 = fileno(infile);
                                    fnum2 = fileno(stdin);
                                    dup2(fnum1, fnum2);
                                    execvp(args2[0], args2);
                                    fclose(infile);
                                    exit(-1);

                                }
                                new_process_group(pid_fork);

                                if (!background) {

                                    set_terminal(pid_fork);

                                    pid_wait = waitpid(pid_fork, &status, WUNTRACED);

                                    set_terminal(getpid());

                                    status_res = analyze_status(status, &info);

                                    add_job(history, new_job(pid_fork, args[0], FOREGROUND));

                                    if (status_res == SUSPENDED) {

                                        block_SIGCHLD();
                                        aux = new_job(pid_fork, args[0], STOPPED);
                                        add_job(jobs, aux);
                                        unblock_SIGCHLD();

                                        printf("\n---Status: %s.---\n", status_strings[status_res]);
                                        fflush(stdout);
                                    }

                                    printf("\n---Foreground pid:%d, comando: %s , status: %s, info: %d.---\n", pid_fork, args[0], status_strings[status_res], info);
                                    fflush(stdout);

                                } else {

                                    add_job(history, new_job(pid_fork, args[0], BACKGROUND));
                                    block_SIGCHLD();
                                    aux = new_job(pid_fork, args[0], BACKGROUND);
                                    add_job(jobs, aux);
                                    unblock_SIGCHLD();

                                    printf("\n---Background pid: %d, comando: %s.---\n", pid_fork, args[0]);
                                    fflush(stdout);
                                }
                                break;
                        }

                    }
                } else {
                    restore_terminal_signals();
                    execvp(args [0], args);
                    printf("---Error: comando %s no encontrado.---", args[0]);
                    exit(-1);

                }
                break;

            default:
                new_process_group(pid_fork);

                if (!background) {

                    set_terminal(pid_fork);

                    pid_wait = waitpid(pid_fork, &status, WUNTRACED);

                    set_terminal(getpid());

                    status_res = analyze_status(status, &info);

                    add_job(history, new_job(pid_fork, args[0], FOREGROUND));

                    if (status_res == SUSPENDED) {

                        block_SIGCHLD();
                        aux = new_job(pid_fork, args[0], STOPPED);
                        add_job(jobs, aux);
                        unblock_SIGCHLD();

                        printf("\n---Status: %s.---\n", status_strings[status_res]);
                        fflush(stdout);
                    }

                    printf("\n---Foreground pid:%d, comando: %s , status: %s, info: %d.---\n", pid_fork, args[0], status_strings[status_res], info);
                    fflush(stdout);

                } else {

                    add_job(history, new_job(pid_fork, args[0], BACKGROUND));
                    block_SIGCHLD();
                    aux = new_job(pid_fork, args[0], BACKGROUND);
                    add_job(jobs, aux);
                    unblock_SIGCHLD();

                    printf("\n---Background pid: %d, comando: %s.---\n", pid_fork, args[0]);
                    fflush(stdout);
                }
                break;
        }
    }
}
