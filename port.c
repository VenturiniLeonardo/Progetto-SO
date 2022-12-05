#define _GNU_SOURCE
//System libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* For portability */
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <float.h> /*for double compare in arrContains*/
#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>

//Own libraries or definitions
#include "config.h"
#include "struct.h"

//Function declaration
int generatorDock();
void generatorSupply();

//Handler
void alarmHandler(int sig){
    generatorSupply();
}

struct supplyDemand *sd;

//Port Main
int main(int argc, char **argv){
    int nDocks = generatorDock();
    int currFill = 0;

    //Semaphore creation
    int dockSem;
    if((dockSem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0600)) == -1){
        fprintf(stderr,"Error docks semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if(semctl(dockSem,0,SETVAL,nDocks)<0){
        fprintf(stderr,"Error initializing docks semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sigaction sAllarm;
    
    bzero(&sAllarm, sizeof(sAllarm));
    sAllarm.sa_handler = alarmHandler;
    if(sigaction(SIGALRM,&sAllarm,NULL) == -1){
        printf("Errore Handler Allar\n");
        exit(EXIT_FAILURE);
    }

    int shmPort;
    if((shmPort = shmget(getpid(),sizeof(sd),IPC_CREAT |IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    sd =  (struct supplyDemand *) shmat(shmPort,NULL,0);
    if ( sd == (void *) -1){
        fprintf(stderr,"Error assing goods to ports shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Sy Semaphore*/
    int sySem;
    if((sySem = semget(getppid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    struct sembuf sops;
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 


    


}


//Functions definitions

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int generatorDock(){
    srand(time(NULL));
    return (rand()%SO_BANCHINE)+1;
}

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
void generatorSupply(){

    struct good newGood;
    newGood.quantity = SO_FILL/SO_DAYS;
    newGood.type = (rand() % SO_MERCI)+1;
    newGood.date_expiry = (rand()%SO_MAX_VITA)+SO_MIN_VITA;

    
    
}
