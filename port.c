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
int generatorDemand();
int generatorSupply();
void reloadExpiryDate();

//Handler
void alarmHandler(int sig){
    if(generatorSupply()){
        printf(stderr,"Error queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    reloadExpiryDate();
}

int mqDemand;
int typeGoods[SO_MERCI];
int typeGoodsOffer[SO_MERCI];
int quantityDemand;
int quantitySupply;

//Port Main
int main(int argc, char **argv){
    int nDocks = generatorDock();
    int currFill = 0;
    int quantityDemand = 0;
    int quantitySupply = 0;
    int shid_offer;
    

    //SHM creation for offer
    /*
    shid_offer=shmget(getpid(),SO_MERCI*sizeof(good), S_IRUSR | S_IWUSR);
    if(shid_offer==-1){
        fprintf(stderr, "%s: %d. Error in shmget #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    exit(EXIT_FAILURE);
    }

    
    */

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
    

    //MESSAGGE QUEUE FOR DEMAND
    if((mqDemand = msgget(getpid(),IPC_CREAT | IPC_EXCL | 0666)) == -1){
        fprintf(stderr,"Error initializing messagge queue for demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    //GENERATE SUPPLY AND DEMAND

    if(generatorSupply()){
        printf(stderr,"Error queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(generatorDemand()){
        printf(stderr,"Error queue demand, %d: %s\n",errno,strerror(errno));
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
Output: void
Desc: rgenerate every day a supply
*/
int generatorSupply(){

    struct good newGood;
    int i = 0;
    do{
        newGood.type = (rand() % SO_MERCI)+1;
        i++;
    }while(typeGoods[newGood.type] == 1 && i < SO_MERCI);
    
    typeGoods[newGood.type] = 1;

    if(i == SO_MERCI){
        return -1;
    }
    
    newGood.quantity = SO_FILL/SO_DAYS;
    if(newGood.quantity > quantitySupply)
        return -1;
    quantitySupply += newGood.quantity;
    newGood.date_expiry = (rand()%SO_MAX_VITA-SO_MIN_VITA)+SO_MIN_VITA;
    

}

/*
Input: void
Output: void
Desc: reload expiry date to every goods
*/
void reloadExpiryDate(){
    int i;

}

/*
Input: void
Output: void
Desc: reload expiry date to every goods
*/
int generatorDemand(){
    struct msgDemand msg;
    int i;
    do{
        msg.type = (rand() % SO_MERCI)+1;
        i++;
    }while(typeGoods[msg.type] == 1 && i < SO_MERCI);
    
    if(i == SO_MERCI){
        return -1;
    }
    
    typeGoods[msg.type] = 1;

    msg.quantity = (rand() % (quantityDemand-SO_FILL))+1;
    quantityDemand += msg.quantity;

    int sndDemand;
    if(sndDemand = msgsnd(mqDemand,&msg,sizeof(int),0) == -1){
        fprintf(stderr,"Error send messagge queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;

}