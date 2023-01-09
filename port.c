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
int blockAllDock();
void unblockAllDock(int);
void stop_ships();
void swell();
void generatorDailySupply();

struct port_states* port_d;
int mqDemand;
int dumpSem;
/*struct good offers[SO_MERCI];*/   
int quantityDemand;
int quantitySupply;
struct shmSinglePort* shmPort;
struct goods_states * good_d;
int my_index;
struct ship_condition * ships;
int msg_generator_supply;
struct good_info *info_goods;
/*Handler*/
void signalHandler(int signal){
    switch(signal){
        case SIGUSR2:
            swell();
        break;
        case SIGUSR1:
            reloadExpiryDate();
        break;
        case SIGALRM:
            generatorDailySupply();
        break;
        case SIGTERM:
            deallocateResources();
            exit(EXIT_SUCCESS);
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
    int shmShip;
    int shmGoods;
    
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

    /*Dump for port*/
    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_states),0666))==-1)
        TEST_ERROR;

    port_d=(struct port_states*)shmat(shm_dump_port,NULL,0);
    port_d[my_index].goods_demand = 0;
    port_d[my_index].goods_offer = 0;
    port_d[my_index].goods_receved = 0;
    port_d[my_index].goods_sended = 0;
    port_d[my_index].swell = 0;
    port_d[my_index].dock_occuped = 0;
    port_d[my_index].dock_total = nDocks;

    /*Dump  for goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_states),0666 )) == -1){
        fprintf(stderr,"Error shared memory good dump creation in port, %d: %s\n",errno,strerror(errno));
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
    if((semSupply=semget(getpid()*2,1,IPC_CREAT|IPC_EXCL|0666)) == 1)
        TEST_ERROR;
    /*printf(" --> %d %d\n",key_semSupply,semSupply);*/
    if(semctl(semSupply,0,SETVAL,1)==-1){
        TEST_ERROR;
    }

    /*MESSAGGE QUEUE FOR DEMAND*/
    if((mqDemand = msgget(getpid(),IPC_CREAT | IPC_EXCL |0666)) == -1){
        fprintf(stderr,"Error initializing messagge queue for demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*SHM creation for offer*/  
    if((shm_offer=shmget(getpid(),sizeof(struct shmSinglePort)*SO_MERCI, IPC_CREAT | IPC_EXCL | 0666)) == -1)
        TEST_ERROR;  
    shmPort = (struct shmSinglePort *)shmat(shm_offer, NULL, 0);
    if(shmPort == (void *) -1){
        TEST_ERROR;
    }

    /*shm for ships attached to a port */
    if((shmShip = shmget(SHIP_POS_KEY,0,0666)) == -1){
        TEST_ERROR;
    }
    ships =shmat(shmShip,NULL,SHM_RDONLY);

    /*MSG queue for generator daily supply */
    if((msg_generator_supply = msgget(getppid(),0666)) == -1){
        fprintf(stderr,"Error initializing messagge queue for demand, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }


    if((shmGoods = shmget(GOODS_INFO_KEY,sizeof(0),0666)) == -1)
        TEST_ERROR;
    
    info_goods = (struct good_info*) shmat(shmGoods,NULL,0);


    quantityDemand=0;
    quantitySupply=0;

    generatorSupply();
    generatorDemand();
        
    bzero(&sa, sizeof(sa));
    sa.sa_handler = signalHandler;
    sa.sa_flags= SA_RESTART|SA_NODEFER;
    sigaction(SIGUSR2,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);
    sigaction(SIGALRM,&sa,NULL);
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
    }while(1);
}


/*Functions definitions*/

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
void generatorDailySupply(){

    struct msgSupply msg_Supply;
    int date_expry;
    key_t key_semSupply;
    int semSupply;
    struct sembuf sops;
    struct sembuf sops_dump;
    msgrcv(msg_generator_supply,&msg_Supply,sizeof(struct msgSupply)-sizeof(long),getpid(),0);

    if((semSupply=semget(getpid()*2,1,0666))==-1){
        TEST_ERROR;
    }
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(semSupply,&sops,1);
    sops_dump.sem_num=0;
    sops_dump.sem_op=-1;
    sops_dump.sem_flg=0;
    semop(dumpSem,&sops_dump,1);
    if(shmPort[msg_Supply.type-1].demandGoods==0 && port_d[my_index].goods_offer+msg_Supply.quantity*info_goods[msg_Supply.type-1].size<=SO_FILL){
        shmPort[msg_Supply.type-1].supplyGoods=1;
        shmPort[msg_Supply.type-1].supply.type=msg_Supply.type;
        shmPort[msg_Supply.type-1].supply.quantity=msg_Supply.quantity;

        shmPort[msg_Supply.type-1].supply.date_expiry=info_goods[msg_Supply.type-1].life;
        good_d[msg_Supply.type-1].goods_in_port += msg_Supply.quantity*info_goods[msg_Supply.type-1].size;
        port_d[my_index].goods_offer += msg_Supply.quantity*info_goods[msg_Supply.type-1].size;
    }
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);
    sops.sem_op=1;
    semop(semSupply,&sops,1);
}

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int generatorDock(){
    srand(getpid());
    return (rand()%SO_BANCHINE)+1;
}

/*
Input: void
Output: intpid_port
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


    if((semSupply=semget(getpid()*2,1,0666))==-1)
        TEST_ERROR;

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;

    semop(semSupply,&sops,1);

    srand(getpid());
    newGood.type = (rand() % SO_MERCI)+1;
    shmPort[newGood.type-1].supplyGoods = 1;
    newGood.quantity = (SO_FILL/SO_PORTI)/info_goods[newGood.type-1].size;

    sops_dump.sem_num=0;
    sops_dump.sem_op=-1;
    sops_dump.sem_flg=0;
    semop(dumpSem,&sops_dump,1);

    good_d[newGood.type-1].goods_in_port += newGood.quantity*info_goods[newGood.type-1].size;
    port_d[my_index].goods_offer+=newGood.quantity*info_goods[newGood.type-1].size;
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    newGood.date_expiry = info_goods[newGood.type-1].life;
    shmPort[newGood.type-1].supply.quantity = newGood.quantity;
    shmPort[newGood.type-1].supply.date_expiry = newGood.date_expiry;
    shmPort[newGood.type-1].supply.type = newGood.type;
    printf("Generata offerta porto %d t: %d q: %d s: %d\n",getpid(),newGood.type,shmPort[newGood.type-1].supply.quantity ,shmPort[newGood.type-1].supply.date_expiry);
        
    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    semop(semSupply,&sops,1);
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

    if((semSupply=semget(getpid()*2,1,0666)) == -1)
        TEST_ERROR;
    

    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(semSupply,&sops,1);

    for(i=0;i<SO_MERCI;i++){
        if(shmPort[i].supplyGoods == 1){
            if(shmPort[i].supply.date_expiry >= 1)
                shmPort[i].supply.date_expiry -= 1;
            else{
                shmPort[i].supplyGoods= 0;
                shmPort[i].supply.date_expiry = -1;
                sops_dump.sem_num=0;
                sops_dump.sem_op=-1;
                sops_dump.sem_flg=0;
                semop(dumpSem,&sops_dump,1);
                good_d[i].goods_in_port -= shmPort[i].supply.quantity*info_goods[i].size;
                good_d[i].goods_expired_port += shmPort[i].supply.quantity*info_goods[i].size;
                sops_dump.sem_op=1;
                semop(dumpSem,&sops_dump,1);
            }
        }
    }

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    semop(semSupply,&sops,1);

}

/*o
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

    semDemand=semget(getpid()*2,1,0666);
    /*INSERT SUPPLY INTO SHM*/
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    
    semop(semDemand,&sops,1);

    srand(time(NULL));
      
    do{
        msg.type = (rand() % SO_MERCI)+1;
        i++;

    }while((shmPort[msg.type-1].supplyGoods== 1)&& i < SO_MERCI);
    
    if(i == SO_MERCI+1){
        return -1;
    }

    shmPort[msg.type-1].demandGoods = 1;
    msg.quantity =  (SO_FILL/SO_PORTI)/info_goods[msg.type-1].size;
    
    if(sndDemand = msgsnd(mqDemand,&msg,sizeof(int),IPC_NOWAIT) == -1){
        fprintf(stderr,"Error send messagge queue demand port, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*printf("Generata domanda porto %d t: %ld q: %d\n",getpid(),msg.type,msg.quantity);*/
    sops_dump.sem_num=0;
    sops_dump.sem_op=-1;
    sops_dump.sem_flg=0;
    semop(dumpSem,&sops_dump,1);
    port_d[my_index].goods_demand += msg.quantity*info_goods[msg.type-1].size; 
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=0;

    semop(semDemand,&sops,1);

    
    return 0;

}

/*
Input: void
Output: void
Desc: deallocate all resources
*/
int blockAllDock(){
    int semDock;
    int semVal;
    struct sembuf sops;

    if((semDock = semget(getpid(),1,0666)) == -1){
        TEST_ERROR;
    }
    if((semVal = semctl(semDock,0,GETVAL)) > 0){
        sops.sem_num=0;
        sops.sem_op=-semVal;
        sops.sem_flg=0;
        semop(semDock,&sops,1);         
        return semVal;
    }

    return 0;
}

/*
Input: void
Output: void
Desc: deallocate all resources
*/

void unblockAllDock(int semVal){
    int semDock;
    struct sembuf sops;

    if((semDock = semget(getpid(),1,0666)) == -1){
        TEST_ERROR;
    }
    if(semVal != 0){
        sops.sem_num=0;
        sops.sem_op= semVal;
        sops.sem_flg=0;
        semop(semDock,&sops,1);         
    }
}

/*
Input: void
Output: void
Desc: deallocate all resources
*/

void stop_ships(){
    int i;
    int dump_sem_ship;
    struct sembuf ship_dump;
    pid_t port_pid=getpid();
    if((dump_sem_ship = semget(DUMP_KEY_SHIP,1,0666)) == -1){
        exit(EXIT_FAILURE);
    }
    ship_dump.sem_op=-1;
    ship_dump.sem_num=0;
    ship_dump.sem_flg=0;
    semop(dump_sem_ship,&ship_dump,1);
    for(i=0;i<SO_NAVI;i++){
        if(ships[i].port==port_pid){
            /*kill(ships[i].ship,SIGPROF);*/
        }
    }
    ship_dump.sem_op=1;
    semop(dump_sem_ship,&ship_dump,1);
}

/*
Input: void
Output: void
Desc: deallocate all resources
*/

void swell(){
    int semVal = blockAllDock();
    struct timespec req;
    struct timespec rem;
    double time_swell=SO_SWELL_DURATION/24.0;
    stop_ships();
    rem.tv_sec=0;
    rem.tv_nsec=0;
    req.tv_sec=(int)time_swell;
    req.tv_nsec=(time_swell-(int)time_swell)*1000000000;
    while(nanosleep(&req,&rem)<0){
        if(errno!=EINTR){
            TEST_ERROR;
        }else{
            req.tv_sec=rem.tv_sec;
            req.tv_nsec=rem.tv_nsec;
        }
    }
    unblockAllDock(semVal);
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
    int shm_dock;
    key_t supplyKey;

    /*Shm ships/ports*/
    shmdt(ships);

    /*Sem Docks*/
    if((semDock = semget(getpid(),1,0666)) == -1){
        TEST_ERROR;
    }
    if((semctl(semDock,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }

    /*Sem Offer*/
    if((semSupply=semget(getpid()*2,1,0666)) == -1){
        TEST_ERROR;
    }
    if((semctl(semSupply,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }

    /*Shm Offer*/
    if((shm_offer=shmget(getpid(),sizeof(struct shmSinglePort),0666) ) == -1){
        TEST_ERROR;
    }
    
    shmdt(shmPort);

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
    shmdt(info_goods);
    /*Dump*/
    shmdt(port_d);
    shmdt(good_d);    
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
        if(strcmp(variable,"SO_PORTI")== 0){
            SO_PORTI = atoi(value);
            if(SO_PORTI < 4)
                return 1;
        }
        if(strcmp(variable,"SO_BANCHINE")== 0)
            SO_BANCHINE = atoi(value);
        if(strcmp(variable,"SO_FILL")== 0)
            SO_FILL = atoi(value);
        if(strcmp(variable,"SO_NAVI")== 0){
            SO_NAVI = atoi(value);
            if(SO_NAVI < 1)
                return 1;
        }
        if(strcmp(variable,"SO_MERCI")== 0)
            SO_MERCI = atoi(value);
        if(strcmp(variable,"SO_SWELL_DURATION") == 0){
            SO_SWELL_DURATION = atoi(value);
        }

    }

	fclose(f);
    return 0;
}
