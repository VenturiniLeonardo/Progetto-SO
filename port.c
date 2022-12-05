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
void reloadExpiryDate();

//Handler
void alarmHandler(int sig){
    generatorSupply();
    reloadExpiryDate();
}

struct supplyDemandPending *sdp;

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

    //SIGNAL ALARM
    struct sigaction sAllarm;
    
    bzero(&sAllarm, sizeof(sAllarm));
    sAllarm.sa_handler = alarmHandler;
    if(sigaction(SIGALRM,&sAllarm,NULL) == -1){
        printf("Errore Handler Allar\n");
        exit(EXIT_FAILURE);
    }
    
    //SHARED MEMORY
    int shmPort;
    if((shmPort = shmget(getpid(),sizeof(sdp),IPC_CREAT |IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    sdp =  (struct supplyDemandPending *) shmat(shmPort,NULL,0);
    if ( sdp == (void *) -1){
        fprintf(stderr,"Error assing goods to ports shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    sdp->supply  = NULL;
    sdp->demand  = NULL;
    sdp->pending  = NULL;

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
Output: void
Desc: rgenerate every day a supply
*/
void generatorSupply(){

    struct good* newGood = malloc(sizeof(struct good));
    newGood->quantity = SO_FILL/SO_DAYS;
    newGood->type = (rand() % SO_MERCI)+1;
    newGood->date_expiry = (rand()%SO_MAX_VITA)+SO_MIN_VITA;

    struct good* supplyApp = sdp->supply;
    if(sdp->supply == NULL){
        newGood->next = NULL;
        sdp->supply = newGood;
    }else{
        while(supplyApp != NULL && newGood->date_expiry > supplyApp->date_expiry){
            supplyApp = supplyApp->next;
        }

        newGood->next = supplyApp->next;
        supplyApp->next = newGood;
    }

}

/*
Input: void
Output: void
Desc: reload expiry date to every goods
*/
void reloadExpiryDate(){
    int i;
    for(i=0;i<sizeof(supplySize);i++){
        if(sdp->supply[i].date_expiry==0){
            reload_supplyDemandPending();

        }
        else sdp->supply[i].date_expiry -= 1;
        
    }

    for(i=0;i<sizeof(demandSize);i++){
        
        sdp->demand[i].date_expiry -= 1;
    }

    for(i=0;i<sizeof(pendingSize);i++){
        sdp->pending[i].date_expiry -= 1;
    }
}

void reload_supplyDemandPending(){
    struct supplyDemandPending * index=sdp;
    while(index->supply->next!=NULL){
        index->supply=index->supply->next;
        index=index->supply->next;
    }
}