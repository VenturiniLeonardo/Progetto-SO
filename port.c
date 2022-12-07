#define _GNU_SOURCE
/*System libraries*/
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

/*Own libraries or definitions*/
#include "config.h"
#include "struct.h"

/*Function declaration*/
int generatorDock();
int generatorDemand();
int generatorSupply();
void reloadExpiryDate();
void deallocateResources();

int mqDemand;
int typeGoods[SO_MERCI];
/*struct good offers[SO_MERCI];*/
int quantityDemand;
int quantitySupply;
int end = 0;

/*Handler*/
void signalHandler(int signal) {

}


/*Port Main*/
int main(int argc, char **argv){
    int nDocks = generatorDock();
    int currFill = 0;
    int quantityDemand = 0;
    int quantitySupply = 0;
    int shid_offer;
    int dockSem;
    struct sigaction sa;
    sigset_t mask_ALRM_TERM_USR1;
    sigset_t mask;
    struct sembuf sops;
    int sySem;
    int semSupply;
    key_t key_semSupply;

    /*Semaphore creation*/

    dockSem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0666);
        TEST_ERROR;
    
    if(semctl(dockSem,0,SETVAL,nDocks)<0){
        TEST_ERROR;
    }

    /*Sem creation for Supply 
    key_semSupply=ftok("/",getpid());
    semSupply=semget(key_semSupply,1,IPC_CREAT|IPC_EXCL|0666);
    if(semctl(sySem,0,SETVAL,1)==-1){
        TEST_ERROR;
    }*/
  
    /*SIGNAL ALARM
    sigemptyset(&mask_ALRM_TERM_USR1);                 
    sigaddset(&mask_ALRM_TERM_USR1, SIGINT);  
    sigaddset(&mask_ALRM_TERM_USR1, SIGALRM);  
    sigaddset(&mask_ALRM_TERM_USR1, SIGUSR1); 
*/
    bzero(&sa, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigaction(SIGINT,&sa,NULL);
    
   /*  sigprocmask(SIG_SETMASK, &mask_ALRM_TERM_USR1, NULL);*/

    /*MESSAGGE QUEUE FOR DEMAND*/
    if((mqDemand = msgget(getpid(),IPC_CREAT | IPC_EXCL | 0666)) == -1){
        fprintf(stderr,"Error initializing messagge queue for demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*SHM creation for offer*/
    shid_offer=shmget(getpid(),SO_MERCI*sizeof(struct good), IPC_CREAT | IPC_EXCL | 0666);
    TEST_ERROR;
    /*
    offers = (good *)shmat(shmid, 0, 0);
    offers = malloc(sizeof(good)*SO_MERCI);
    */

    /*GENERATE SUPPLY AND DEMAND*/

    if(generatorSupply()){
        fprintf(stderr,"Error queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(generatorDemand()){
        fprintf(stderr,"Error queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Sy Semaphore*/
    if((sySem = semget(getppid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 

    /*WAIT SIGNAL*/
    do{
        sigsuspend(&mask);
    }while(!end);
    

    return 0;
}


/*Functions definitions*/

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int generatorDock(){
    return (rand()%SO_BANCHINE)+1;
}

/*
Input: void
Output: void
Desc: rgenerate every day a supply
*/
int generatorSupply(){
    int shmSupply;
    struct good *supply[SO_MERCI];
    int semSupply;
    struct good newGood;
    int i = 0;
    key_t key_semSupply;
    struct sembuf sops;

    /*key_semSupply=ftok("/",getpid());
    semSupply=semget(key_semSupply,1,IPC_CREAT|IPC_EXCL|0666);
    TEST_ERROR;*/
    


    do{
        newGood.type = (rand() % SO_MERCI)+1;
        i++;
    }while(typeGoods[newGood.type] == 1 && i < SO_MERCI);

    typeGoods[newGood.type] = 1;

    if(i == SO_MERCI){
        return -1;
    }
    
    newGood.quantity = SO_FILL/SO_DAYS;
    if(newGood.quantity > SO_FILL-quantitySupply)
        return -1;
    quantitySupply += newGood.quantity;
    srand(getpid());
    newGood.date_expiry = (rand()%SO_MAX_VITA-SO_MIN_VITA)+SO_MIN_VITA;

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=1;
    sops.sem_op=-1;
    sops.sem_flg=0;
    /*if(semop(ftok("/",getpid()),&sops,1)==-1)
        TEST_ERROR;

    shmSupply = shmget(getpid(),0,IPC_CREAT | IPC_EXCL | 0666);
    supply = (struct good *)shmat(shmSupply,NULL,0);
    if(supply == (void *) -1)
        TEST_ERROR;
    supply[newGood.type] = newGood;*/


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
    int sndDemand;
    do{
        msg.type = (rand() % SO_MERCI)+1;
        i++;
    }while(typeGoods[msg.type] == 1 && i < SO_MERCI);
    
    if(i == SO_MERCI){
        return -1;
    }
    
    typeGoods[msg.type] = 1;
    srand(getpid());
    msg.quantity = (rand() % (quantityDemand-SO_FILL))+1;
    quantityDemand += msg.quantity;

    if(sndDemand = msgsnd(mqDemand,&msg,sizeof(int),0) == -1){
        fprintf(stderr,"Error send messagge queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;

}


/*
Input: void
Output: void
Desc: deallocate all resources
*/
void deallocateResources(){
    int semDock;
    int queMes;

    semDock = semget(getpid(),1,IPC_CREAT | IPC_EXCL|0600);
    semctl(semDock,1,IPC_RMID);
    TEST_ERROR;

    queMes = msgget(getpid(),IPC_CREAT | IPC_EXCL | 0666);
    msgctl(queMes,IPC_RMID,NULL);
    TEST_ERROR;
}