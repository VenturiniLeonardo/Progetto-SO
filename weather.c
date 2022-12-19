/*System libraries*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* For portability*/
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <float.h> /*for double compare in arrContains*/
#include <math.h> /* for abs of value (double compare function)*/
#include <sys/shm.h>
#include <signal.h>

/*Own libraries or definitions*/
#include "config.h"
#include "struct.h"

/*Function declaration*/
int variableUpdate();
void storm();
void swell();
void deallocateResources();

struct port *ports;
struct ship_condition * ships;

/*Handler*/
void signalHandler(int signal){
    switch(signal){
        case SIGUSR2:
            
        break;
        case SIGUSR1:
            
        break;
        case SIGTERM:
            deallocateResources();
            exit(EXIT_SUCCESS);
        break;
    }
}

int main(){
    int sySem;
    int shmPort;
    int shmShip;
    struct sembuf sops; 
    struct sigaction sa;
    sigset_t mask;

    if(variableUpdate()){
        printf("Error set all variable\n");
        return 0;
    }

    /*SHM ports*/
    if((shmPort = shmget(PORT_POS_KEY,0,0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    ports = (struct port*) shmat(shmPort,NULL,0);

    if (ports == (void *) -1){
        fprintf(stderr,"Error assing ports to shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /*SHM ships*/
    if((shmShip = shmget(SHIP_POS_KEY,0,0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(( sySem = semget(SY_KEY,1,0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1);

    sops.sem_op=0;
    semop(sySem,&sops,1);

    bzero(&sa, sizeof(sa));
    sa.sa_handler = signalHandler;
    sa.sa_flags=SA_RESTART||SA_NODEFER;
    sigaction(SIGUSR2,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);

    sigemptyset(&mask);




    return 0;
}
/*Functions definitions*/



/*
Input: void
Output: int
Desc:
*/
void swell(){
    int posPort;
    srand(time(NULL));
    posPort = rand()%SO_PORTI;
    kill(ports[posPort].pidPort,SIGUSR2);
    
}


void storm(){
    int index;
    srand(time(NULL));
    do{
        index=rand()%SO_NAVI;
    }while(ships[index].port!=0);
    kill(ships[index].ship,SIGUSR2);
}

/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/

int variableUpdate(){
    char buffer[256];
	char *variable;
    char * value;
    FILE *f;
    int VarValue;
	f= fopen("variableFile.txt", "r");

    while(fgets(buffer, 256, f) != NULL){
        variable = strtok(buffer, "=");
        value = strtok(NULL, "=");
        if(strcmp(variable,"SO_DAYS") == 0)
            SO_DAYS = atoi(value);
        if(strcmp(variable,"SO_PORTI")== 0){
            SO_PORTI = atoi(value);
            if(SO_PORTI < 4)
                return 1;
        }
        if(strcmp(variable,"SO_BANCHINE")== 0)
            SO_BANCHINE = atoi(value);
        if(strcmp(variable,"SO_FILL")== 0)
            SO_FILL = atoi(value);
        if(strcmp(variable,"SO_LOADSPEED")== 0)
            SO_LOADSPEED = atof(value);
        if(strcmp(variable,"SO_DISTANZA")== 0)
            SO_DISTANZA = atof(value);
        if(strcmp(variable,"SO_NAVI")== 0){
            SO_NAVI = atoi(value);
            if(SO_NAVI < 1)
                return 1;
        }
        if(strcmp(variable,"SO_SPEED")== 0)
            SO_SPEED = atof(value);
        if(strcmp(variable,"SO_CAPACITY")== 0)
            SO_CAPACITY = atoi(value);
        if(strcmp(variable,"SO_MERCI")== 0)
            SO_MERCI = atoi(value);
        if(strcmp(variable,"SO_SIZE")== 0)
            SO_SIZE = atoi(value);
        if(strcmp(variable,"SO_MIN_VITA")== 0)
            SO_MIN_VITA = atoi(value);
        if(strcmp(variable,"SO_MAX_VITA")== 0)
            SO_MAX_VITA = atoi(value);
        if(strcmp(variable,"SO_LATO")== 0){
            SO_LATO = atof(value);
        }
        if(strcmp(variable,"SO_STORM_DURATION") == 0){
            SO_STORM_DURATION = atoi(value);
        }
        if(strcmp(variable,"SO_SWELL_DURATION") == 0){
            SO_SWELL_DURATION = atoi(value);
        }
        if(strcmp(variable,"SO_MEALSTROM") == 0){
            SO_MEALSTROM = atoi(value);
        }    

    }

	fclose(f);
    return 0;
}

/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/
void deallocateResources(){
    shmdt(ships);
    shmdt(ports);
}   