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
int variableUpdate();

struct port_states* port_d;
int mqDemand;
int dumpSem;
/*struct good offers[SO_MERCI];*/
int quantityDemand;
int quantitySupply;
int end = 1;
struct shmSinglePort* shmPort;
struct goods_states * good_d;
int my_index;

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
int main(int argc,char*argv[]){
    int nDocks;
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

    my_index=atoi(argv[1]);
    if(variableUpdate()){
        printf("Error set all variable\n");
        return 0;
    }
    nDocks = generatorDock();
    /*Semaphore creation docks*/

    if((dockSem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        TEST_ERROR;
    }
    if(semctl(dockSem,0,SETVAL,nDocks) == -1){
        TEST_ERROR;
    }

    /*Dump */
    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_states),0666))==-1)
        TEST_ERROR;
    port_d=(struct port_states*)shmat(shm_dump_port,NULL,0);

    port_d[my_index].dock_total = nDocks;
    /*Dump  for goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_states),0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    good_d=(struct goods_states*) shmat(shm_dump_goods,NULL,0);
    
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
    if((shm_offer=shmget(getpid(),sizeof(struct shmSinglePort)*SO_MERCI, IPC_CREAT | IPC_EXCL | 0666)) == -1)
        TEST_ERROR;  
    shmPort = (struct shmSinglePort *)shmat(shm_offer, NULL, 0);
    if(shmPort == (void *) -1)
        TEST_ERROR;
    
    /*GENERATE SUPPLY AND DEMAND*/
    generatorSupply();
    generatorDemand();
        
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
    srand(time(NULL));
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

 
    /*do{
        srand(time(NULL));
        newGood.type = (rand() % SO_MERCI)+1;
        i++;
    }while((shmPort[newGood.type-1].demandGoods == 0 && shmPort[newGood.type-1].supplyGoods == 0) && i < SO_MERCI);
    if(i == SO_MERCI){
        return -1;
    }*/
    srand(getpid());
    newGood.type = (rand() % SO_MERCI)+1;

    shmPort[newGood.type-1].supplyGoods = 1;
    newGood.quantity = (SO_FILL/SO_PORTI)/2;

    sops_dump.sem_op=-1;
    semop(dumpSem,&sops_dump,1);

    /*if(newGood.quantity > SO_FILL-port_d[my_index].goods_offer){
        sops_dump.sem_op=1;
        semop(dumpSem,&sops_dump,1);
        return -1;
    }*/

    good_d[newGood.type-1].goods_in_port += newGood.quantity;
    port_d[my_index].goods_offer+=newGood.quantity;
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    srand(getpid());

    newGood.date_expiry = (rand()%SO_MAX_VITA-SO_MIN_VITA)+SO_MIN_VITA+1;
    shmPort[newGood.type-1].supply.quantity = newGood.quantity;
    shmPort[newGood.type-1].supply.date_expiry = newGood.date_expiry;
    shmPort[newGood.type-1].supply.type = newGood.type;
    printf("Generata offerta porto %d t: %d q: %d s: %d\n",getpid(),newGood.type,shmPort[newGood.type-1].supply.quantity ,shmPort[newGood.type-1].supply.date_expiry);
        
    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    if((semop(semSupply,&sops,1))==-1)
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
    if((semSupply=semget(key_semSupply,1,0666)) == -1)
        TEST_ERROR;
    

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;

    for(i=0;i<SO_MERCI;i++){
        if(shmPort[i].supplyGoods == 1){
            if(shmPort[i].supply.date_expiry > 0)
                shmPort[i].supply.date_expiry--;
            else{
                shmPort[i].supply.date_expiry = -1;
                shmPort[i].supplyGoods= 0;
                sops_dump.sem_op=-1;
                semop(dumpSem,&sops_dump,1);
                good_d[i].goods_in_port -= shmPort[i].supply.quantity;
                good_d[i].goods_expired_port += shmPort[i].supply.quantity;
                sops_dump.sem_op=1;
                semop(dumpSem,&sops_dump,1);
            }
        }
    }

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    if(semop(semSupply,&sops,1) == -1)
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
    
    if((semop(semDemand,&sops,1))==-1)
        TEST_ERROR;
    srand(time(NULL));
      
    do{
        msg.type = (rand() % SO_MERCI)+1;
        i++;

    }while((shmPort[msg.type-1].supplyGoods== 1)&& i < SO_MERCI);
    
    if(i == SO_MERCI+1){
        return -1;
    }
    shmPort[msg.type-1].demandGoods = 1;
    srand(time(NULL));
    msg.quantity =  (SO_FILL/SO_PORTI)/2;
    quantityDemand += msg.quantity;
    
    if(sndDemand = msgsnd(mqDemand,&msg,sizeof(int),IPC_NOWAIT) == -1){
        fprintf(stderr,"Error send messagge queue demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Generata domanda porto %d t: %ld q: %d\n",getpid(),msg.type,msg.quantity);
    sops_dump.sem_num=0;
    sops_dump.sem_op=-1;
    sops_dump.sem_flg=0;
    semop(dumpSem,&sops_dump,1);
                       
    port_d[my_index].goods_demand += msg.quantity; 
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    if(semop(semDemand,&sops,1) == -1)
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
    if((semctl(semSupply,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }

    /*Shm Offer*/
    if((shm_offer=shmget(getpid(),sizeof(struct shmSinglePort),0666) ) == -1){
        TEST_ERROR;
    }
    /*MANCA FREE                                                   ---------------------------------------------*/
    if((shmctl(shm_offer,IPC_RMID,0)) == -1){
        TEST_ERROR;
    }

    /*Msg Queue*/
    if((queMes = msgget(getpid(),0666)) == -1){
        TEST_ERROR;
    }
    if((msgctl(queMes,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }
    
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
            

    }

	fclose(f);
    return 0;
}