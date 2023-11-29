#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>


#define bool char
#define true 1
#define false 0


typedef struct {
    int clients;
    int staff;
    int client_wait;
    int break_time;
    int close_time;
} parameter;

parameter *parsing_arguments(int count, char **arguments){
    if (count != 6){
        return NULL;
    }
    parameter *result = (parameter *)malloc(sizeof(parameter)*count);
    result->clients = (int)strtol(arguments[1], NULL, 10);
    result->staff = (int)strtol(arguments[2], NULL, 10);
    result->client_wait = (int)strtol(arguments[3], NULL, 10);
    result->break_time = (int)strtol(arguments[4], NULL, 10);
    result->close_time = (int)strtol(arguments[5], NULL, 10);
    if (!(result->client_wait <= 10000 && result->client_wait >= 0  &&
            result->break_time <= 100 && result->break_time >= 0
    &&      result->close_time <= 10000 && result->close_time >= 0 &&
            result->staff > 0 && result->clients > 0)){
        free(result);
        return NULL;
    }
    return result;
}

int random_sleep(int maxTime) {
    if (maxTime == 0) {
        return 0;
    }
    srand((unsigned) time(NULL) + getpid());
    return rand() % (maxTime*1000);
}

int random_cinnost() {
    srand((unsigned) time(NULL) + getpid());
    return (rand() % 3) + 1;
}

int get_rand_quene(int *pole, int size){
    int dom = rand() % size;
    int result = pole[dom];
    for (int i = dom; i < size; ++i) {
        pole[i] = pole[i+1];
    }
    pole[size] = 0;
    return result;
}

void unlink_all(){
    sem_unlink("/mysemaphore"); // delete semaphore
    sem_unlink("/zakaz");
    sem_unlink("/ured");
    sem_unlink("/ured_done");
    sem_unlink("/zakaz_done");
    sem_unlink("/home");
    sem_unlink("/quene");
}


int main(int argc, char **argv){
    FILE *file = fopen("proj2.out", "w+");
    unlink_all();
    sem_t *mutex = sem_open("/mysemaphore", O_CREAT, 0644, 1); // create semaphore
    sem_t *zakaz = sem_open("/zakaz", O_CREAT, 0644, 0);
    sem_t *ured = sem_open("/ured", O_CREAT, 0644, 0);
    sem_t *ured_done = sem_open("/ured_done", O_CREAT, 0644, 0);
    sem_t *zakaz_done = sem_open("/zakaz_done", O_CREAT, 0644, 0);
    sem_t *home = sem_open("/home", O_CREAT, 0644, 1);
    sem_t *quene = sem_open("/quene", O_CREAT, 0644, 0);
    if (mutex == SEM_FAILED || zakaz == SEM_FAILED || ured == SEM_FAILED || ured_done == SEM_FAILED || zakaz_done == SEM_FAILED ||
        home == SEM_FAILED  || quene == SEM_FAILED ) {
        perror("sem_open() failed");
        exit(EXIT_FAILURE);
    }
    parameter *params = parsing_arguments(argc, argv);
    if (params == NULL){
        fprintf(stderr, "Invalid arguments\n");
        exit(EXIT_FAILURE);
    }
    bool *posta_otevrena = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *shared_var = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *first  = mmap(NULL, sizeof(int)*params->clients, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *second = mmap(NULL, sizeof(int)*params->clients, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *third  = mmap(NULL, sizeof(int)*params->clients, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *one = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *two = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *three = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *clients = malloc(sizeof(int)*params->clients);
    int *staff = malloc(sizeof(int)*params->staff);
    int status;
    if (clients == NULL || staff == NULL){
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    *one = 0, *two = 0, *three = 0, *posta_otevrena = true, *shared_var = 0;
    if (pid == 0){

        for (int i = 0; i < params->clients; i++) {
            int cinnost;
            clients[i] = fork();
            if (clients[i] < 0) { // error occurred
                perror("fork() failed");
                exit(EXIT_FAILURE);
            }
            else if (clients[i] == 0) { // child process
                fprintf(file,"%d: Z %d: started\n", ++(*shared_var) , i+1);
                fflush(file);
                usleep(random_sleep(params->client_wait));
                sem_wait(mutex);
                if (!*posta_otevrena){
                    fprintf(file,"%d: Z %d: going home\n", ++(*shared_var) , i+1);
                    fflush(file);
                    sem_post(mutex);
                    exit(EXIT_SUCCESS);
                }
                cinnost = random_cinnost();
                if(cinnost == 1){ first[(*one)++] = getpid(); }
                else if (cinnost == 2){ second[(*two)++] = getpid(); }
                else if (cinnost == 3){ third[(*three)++] = getpid(); }
                fprintf(file,"%d: Z %d: entering office for a service %d\n", ++(*shared_var), i+1,  cinnost);
                fflush(file);
                sem_post(zakaz);
                sem_wait(ured);
                fprintf(file,"%d: Z %d: called by office worker\n", ++(*shared_var) , i+1);
                fflush(file);
                usleep(rand() % 10);
                sem_post(zakaz_done);
                sem_wait(ured_done);
                fprintf(file,"%d: Z %d: going home\n", ++(*shared_var) , i+1);
                fflush(file);
                exit(EXIT_SUCCESS);
            }
        }
        for (int i = 0; i < params->clients; i++) {
            waitpid(clients[i], &status, 0); // wait for child processes to finish
        }
    }
    else {
        int ppid = fork();
        if (ppid == 0){
            for (int i = 0; i < params->staff; i++) {
                int cinnost;
                int get_quene;
                int actual_client;
                staff[i] = fork(); // create child process
                if (staff[i] < 0) { // error occurred
                    perror("fork() failed");
                    exit(1);
                }
                else if (staff[i] == 0) { // child process
                    fprintf(file,"%d: U %d: started\n", ++(*shared_var) , i+1);
                    fflush(file);
                    while (1){
                        actual_client = 0;
                        sem_wait(home);
                        cinnost = random_cinnost();
                        sem_getvalue(zakaz, &get_quene);
                        while ((!get_quene) && *posta_otevrena){
                            sem_getvalue(zakaz, &get_quene);
                        }
                        if(!*posta_otevrena && (!get_quene)){
                            fprintf(file,"%d: U %d: going home\n", ++(*shared_var), i+1);
                            fflush(file);
                            sem_post(home);
                            break;
                        }
                        sem_post(ured);
                        sem_wait(zakaz);
                        while (actual_client == 0){
                            if (cinnost == 1) {
                                if (*one <= 0 || one == NULL) { cinnost = cinnost + 1; }
                                else { actual_client = get_rand_quene(first, (*one)--);}
                            }
                            if (cinnost == 2) {
                                if (*two <= 0 || two == NULL) { cinnost = cinnost + 1; }
                                else { actual_client = get_rand_quene(second, (*two)--); }
                            }
                            if (cinnost == 3) {
                                if (*three <= 0 || three == NULL) { cinnost = 1; }
                                else { actual_client = get_rand_quene(third, (*three)--); }
                            }
                        }
                        sem_post(mutex);
                        fprintf(file,"%d: U %d: serving a service of type %d\n", ++(*shared_var) , i+1, cinnost);
                        fflush(file);
                        usleep(rand() % 10);
                        fprintf(file,"%d: U %d: service finished\n", ++(*shared_var) , i+1);
                        fflush(file);
                        sem_wait(zakaz_done);
                        sem_post(ured_done);
                        sem_post(home);
                        sem_getvalue(zakaz, &get_quene);
                        if (!get_quene && *posta_otevrena){
                            fprintf(file,"%d: U %d: taking break\n", ++(*shared_var) , i+1);
                            fflush(file);
                            usleep(random_sleep(params->break_time));
                            fprintf(file,"%d: U %d: break finished\n", ++(*shared_var) , i+1);
                            fflush(file);
                        }
                    }// child process
                    exit(EXIT_SUCCESS);
                }
            }
            for (int i = 0; i < params->clients; i++) {
                waitpid(staff[i], &status, 0); // wait for child processes to finish
            }
            exit(0);
        }else{
            if (*posta_otevrena) {
                usleep(params->close_time * 1000);
                fprintf(file,"%d: closing\n", ++(*shared_var));
                fflush(file);
                *posta_otevrena = false;
                wait(NULL);
            }
        }
    }
    wait(NULL);
    if (pid != 0){
        if (munmap(first, sizeof(int)*params->clients) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(second, sizeof(int)*params->clients) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(third, sizeof(int)*params->clients) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(one, sizeof(int)) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(two, sizeof(int)) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(three, sizeof(int)) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(shared_var, sizeof(int)) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        if (munmap(posta_otevrena, sizeof(bool)) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
        sem_close(mutex); // close semaphore
        sem_close(zakaz_done);
        sem_close(zakaz);
        sem_close(ured);
        sem_close(ured_done);
        sem_close(home);
        sem_close(quene);
        unlink_all();
        free(clients);
        free(staff);
        free(params);
        fclose(file);
        system("sort -n -k1 -o proj2.out proj2.out");
    }
    return 0;
}
