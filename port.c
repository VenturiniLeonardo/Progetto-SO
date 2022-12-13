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

struct port_dump* port_d;
int mqDemand;
int dumpSem;
/*struct good offers[SO_MERCI];*/
int quantityDemand;
int quantitySupply;
int end = 1;
struct shmSinglePort* shmPort;
struct goods_dump * good_d;


/*Handler*/
void signalHandler(int signal){
    switch(signal){
        case SIGALRM:
            printf("SIGALARM\n");
        break;
        case SIGUSR1:
            reloadExpiryDate();
        break;
        case SIGTERM:
            end = 0;
        break;
    }
}


/*Port Main*/
int main(){
    int nDocks = generatorDock();
    int currFill = 0;
    int dockSem;
    struct sigaction sa;
    sigset_t mask;
    struct sembuf sops;
    int sySem;
    int semSupply;
    key_t key_semSupply;
    key_t key_msgDemand;
    int shm_offer;
    int i;
    int j;
    int shm_dump_port;
    int shm_dump_goods;
    quantityDemand = 0;
    quantitySupply = 0;
    
    /*Semaphore creation docks*/
    dockSem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0666);
    TEST_ERROR;
    if(semctl(dockSem,0,SETVAL,nDocks)<0){
        TEST_ERROR;
    }

    /*Dump */
    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_dump),0666))==-1)
        TEST_ERROR;
    port_d=(struct port_dump*)shmat(shm_dump_port,NULL,0);
        TEST_ERROR;

    /*Dump  for goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_dump),0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    good_d=(struct goods_dump*) shmat(shm_dump_goods,NULL,0);
    
    TEST_ERROR;

    /*Sem for dump*/
    if((dumpSem = semget(DUMP_KEY,1,0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Sem creation for Supply */
    key_semSupply=ftok("port.c",getpid());
    semSupply=semget(key_semSupply,1,IPC_CREAT|IPC_EXCL|0666);
    if(semctl(semSupply,0,SETVAL,1)==-1){
        TEST_ERROR;
    }

    /*MESSAGGE QUEUE FOR DEMAND*/
    if((mqDemand = msgget(getpid(),IPC_CREAT | 0666)) == -1){
        fprintf(stderr,"Error initializing messagge queue for demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*SHM creation for offer*/
    shm_offer=shmget(getpid(),sizeof(struct shmSinglePort), IPC_CREAT | IPC_EXCL | 0666);
    TEST_ERROR;  
    shmPort = (struct shmSinglePort *)shmat(shm_offer, NULL, 0);
    if(shmPort == (void *) -1)
        TEST_ERROR;
    for(i=0;i<SO_MERCI;i++){
        for(j=0;j<2;j++){
            shmPort->typeGoods[i][j]=0;
        }
    }
    /*GENERATE SUPPLY AND DEMAND*/
    if(generatorSupply()){
        printf("Error generator supply\n");
    }
    if(generatorDemand()){
        printf("Error generator demand\n");
    }



    /*Handler*/
        
    bzero(&sa, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigaction(SIGALRM,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);

    sigemptyset(&mask);
    
    
    /*Sy Semaphore*/
    if((sySem = semget(SY_KEY,1,0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 

    sops.sem_op=0;
    semop(sySem,&sops,1);
    

    /*WAIT SIGNAL*/
    do{
        sigsuspend(&mask);
    }while(end);

    deallocateResources();
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
Output: int
Desc: returns 0 if it generated goods, -1 if it is not possible and 1 otherwise
*/
int generatorSupply(){
    int shmSupply;
    int semSupply;
    struct good newGood;
    int i = 0;
    key_t key_semSupply;
    struct sembuf sops;
    struct sembuf sops_dump;
    key_semSupply=ftok("port.c",getpid());
    if((semSupply=semget(key_semSupply,1,0666))==-1)
        TEST_ERROR;

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;

    if((semop(semSupply,&sops,1))==-1)
        TEST_ERROR;


    do{
        srand(getpid());
        newGood.type = (rand() % SO_MERCI)+1;
        i++;
    }while((shmPort->typeGoods[newGood.type-1][1] == 1 || shmPort->typeGoods[newGood.type-1][0] == 1) && i < SO_MERCI);


    if(i == SO_MERCI){
        return -1;
    }

    shmPort->typeGoods[newGood.type-1][0] = 1;
    newGood.quantity = SO_FILL/SO_DAYS;
    
    if(newGood.quantity > SO_FILL-quantitySupply)
        return -1;
    


    sops_dump.sem_op=-1;
    semop(dumpSem,&sops_dump,1);
    good_d->states[newGood.type-1].goods_in_port += 1;
    port_d->states[getpid()].goods_offer+=newGood.quantity; 
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    quantitySupply += newGood.quantity;
    srand(getpid());
    newGood.date_expiry = 3;/*(rand()%SO_MAX_VITA-SO_MIN_VITA)+SO_MIN_VITA;*/
    shmPort->supply[newGood.type-1].quantity = newGood.quantity;
    shmPort->supply[newGood.type-1].date_expiry = newGood.date_expiry;
    shmPort->supply[newGood.type-1].type = newGood.type;
    printf("Generata offerta porto %d t: %d q: %d s: %d\n",getpid(),newGood.type,shmPort->supply[newGood.type-1].quantity,shmPort->supply[newGood.type-1].date_expiry);

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    semop(semSupply,&sops,1);
    TEST_ERROR;

    return 0;

}

/*
Input: void
Output: void
Desc: reload expiry date to every goods
*/
void reloadExpiryDate(){
    int i;
    int j; 
    key_t key_semSupply;
    int semSupply;
    struct sembuf sops;
    struct sembuf sops_dump;

    key_semSupply=ftok("port.c",getpid());
    semSupply=semget(key_semSupply,1,0666);
    TEST_ERROR;
    

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;

    semop(semSupply,&sops,1);
    TEST_ERROR;

    for(i=0;i<SO_MERCI;i++){
        if(shmPort->typeGoods[i][0] == 1){
            if(shmPort->supply[i].date_expiry > 0)
                shmPort->supply[i].date_expiry--;
            else{
                shmPort->supply[i].date_expiry = -1;
                shmPort->typeGoods[i][0] == 0;
                sops_dump.sem_op=-1;
                semop(dumpSem,&sops_dump,1);
                good_d->states[i].goods_in_port -= 1;
                good_d->states[i].goods_expired_port += 1;
                sops_dump.sem_op=1;
                semop(dumpSem,&sops_dump,1);
            }
        }
    }

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    semop(semSupply,&sops,1);
    TEST_ERROR;

}

/*
Input: void
Output: int
Desc: returns 0 if it generated demand, -1 if it is not possible and 1 otherwise
*/
int generatorDemand(){
    struct msgDemand msg;
    int i;
    int sndDemand;
    struct sembuf sops;
    struct sembuf sops_dump;
    int semDemand;
    key_t key_semDemand;

    key_semDemand=ftok("port.c",getpid());
    semDemand=semget(key_semDemand,1,0666);

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    
    semop(semDemand,&sops,1);
    TEST_ERROR;

    srand(time(NULL));
    
    do{
        msg.type = (rand() % SO_MERCI)+1;
        i++;
    }while((shmPort->typeGoods[msg.type-1][0] == 1 || shmPort->typeGoods[msg.type-1][1] == 1 )&& i < SO_MERCI);
    
    if(i == SO_MERCI){
        return -1;
    }
    
    shmPort->typeGoods[msg.type-1][1] = 1;
    srand(time(NULL));

    msg.quantity = (rand() % (SO_FILL-quantityDemand))+1;
    quantityDemand += msg.quantity;

    if(sndDemand = msgsnd(mqDemand,&msg,sizeof(int),IPC_NOWAIT) == -1){
        fprintf(stderr,"Error send messagge queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Generata domanda porto %d t: %ld q: %d\n",getpid(),msg.type,msg.quantity);


    sops_dump.sem_op=-1;
    semop(dumpSem,&sops_dump,1);
    port_d->states[getpid()].goods_demand += msg.quantity; 
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    semop(semDemand,&sops,1);
    TEST_ERROR;
    
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
    int shm_offer;
    int semSupply;

    /*Sem Docks*/
    if((semDock = semget(getpid(),1,0666)) == -1){
        TEST_ERROR;
    }
    if((semctl(semDock,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }

    /*Sem Offer*/
    if((semSupply=semget(ftok("port.c",getpid()),1,0666)) == -1){
        TEST_ERROR;
    }
    semctl(semSupply,0,IPC_RMID);
    TEST_ERROR;

    /*Shm Offer*/
    if((shm_offer=shmget(getpid(),sizeof(struct shmSinglePort),0666) ) == -1){
        TEST_ERROR;
    }
    shmctl(shm_offer,IPC_RMID,0);
    TEST_ERROR;

    /*Msg Queue*/
    if((queMes = msgget(getpid(),0666)) == -1){
        TEST_ERROR;
    }

    msgctl(queMes,IPC_RMID,NULL);
    TEST_ERROR;
    
} 