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
int arrContains(struct port[],struct coords,int );
int portGenerator();
int shipGenerator();
int arrContains(struct port [],struct coords ,int );
int deallocateResources();
int genRandInt(int,int);
int printDump(int,struct goods_dump *,struct port_dump *,struct ship_dump *);

struct port *ports; 

/*Master main*/
int main(){
    
    int sySem;
    int sem_dump;
    int shm_dump_goods;
    struct goods_dump* struct_goods_dump;
    int shm_dump_port;
    struct port_dump* struct_port_dump;
    int shm_dump_ship;
    struct ship_dump* struct_ship_dump;
    int shmPort;
    int index;
    int elapsedDays;
    int nRandPort;
    int RandPort;
    int i;
    int dSem;
    struct sembuf sops; 

    checkParams();
    /*Semaphore for sync*/
    if((sySem = semget(SY_KEY,1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(semctl(sySem,0,SETVAL,SO_NAVI+SO_PORTI+1)<0){
        fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Sem for dump*/
    if((dSem = semget(DUMP_KEY,1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Shared memory for dump of goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_dump),IPC_CREAT |IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct_goods_dump=(struct goods_dump*) shmat(shm_dump_goods,NULL,0);
    TEST_ERROR;

    struct_goods_dump->states->goods_delivered = 0;
    struct_goods_dump->states->goods_expired_port = 0;
    struct_goods_dump->states->goods_expired_ship = 0;
    struct_goods_dump->states->goods_in_port = 0;
    struct_goods_dump->states->goods_on_ship = 0;

    /*Shared memory for dump of port*/

     if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_dump),IPC_CREAT |IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct_port_dump=(struct port_dump*)shmat(shm_dump_port,NULL,0);
    TEST_ERROR;

    struct_port_dump->states->dock_occuped = 0;
    struct_port_dump->states->dock_total = 0;
    struct_port_dump->states->goods_offer = 0;
    struct_port_dump->states->goods_receved = 0;
    struct_port_dump->states->goods_sended = 0;

    /*Shared memory for dump of ship*/

     if((shm_dump_ship=shmget(SHIP_DUMP_KEY,sizeof(struct ship_dump),IPC_CREAT |IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct_ship_dump=(struct ship_dump*)shmat(shm_dump_ship,NULL,0);
    TEST_ERROR;


    struct_ship_dump->ship_sea_goods = 0;
    struct_ship_dump->ship_in_port = 0;
    struct_ship_dump->ship_sea_no_goods = 0;

    /*PORT AND SHIP*/

    portGenerator();
    shipGenerator();

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 

    sops.sem_op=0;
    semop(sySem,&sops,1);

    printf("START...\n");
    
    /*elapsed days*/
    elapsedDays=0; 
    srand(time(NULL)); 
    while(elapsedDays<SO_DAYS){
        nRandPort=rand()%SO_PORTI;  
        /*kill(ports[nRandPort].pidPort,SIGALRM);  
        for(i=0;i<nRandPort;i++){ 
            RandPort=rand()%SO_PORTI;
            kill(ports[RandPort].pidPort,SIGALRM);
            
        }*/
        if(printDump(dSem,struct_goods_dump,struct_port_dump,struct_ship_dump))
            killAllPorts();
            elapsedDays = SO_DAYS;
        else{
            elapsedDays++;
            sleep(1);
        }

    }

    /*call dump*/
    deallocateResources();
}

/*Functions definitions*/

/* 
Input: int, int
Output: int
Desc: return random int in range(max-min)
*/
int genRandInt(int max, int min){
    int r;
    srand(getpid());
    r = (rand()%(max-min+1))+min;
    return r;
}

/* 
Input: void
Output: struct coords
Desc: return struct coords of rand coords
*/
struct coords generateRandCoords(){

    struct coords c;
    double div;
    div = RAND_MAX / SO_LATO;
    c.x = rand() / div;
    div = RAND_MAX / SO_LATO;
    c.y = rand() / div;

    return c;
}

/*
Input: struct port[], struct coords,int
Output: int
Desc: returns 0 if coordinate is not present 1 otherwise
*/
int arrContains(struct port arr[],struct coords c,int arrLen){
    int contains=0;
    int i;
    double cX,cY;
    for(i=0;i<arrLen;i++){
        cX=fabs(arr[i].coord.x-c.x);
        cY=fabs(arr[i].coord.y-c.y);
        if((cX<=SO_DISTANZA) && (cY<=SO_DISTANZA)){ 
            contains++;
            printf("found in %d! %f %f \n",i,cX,cY);
            break;
        }
    }

    return contains;
}

/*
Input: void
Output: int
Desc: returns 0 if it has generated all ports correctly, -1 otherwise
*/
int portGenerator(){

    struct coords myC;
    int i;
    int shmPort;
    char x[50];
    char y[50];
    pid_t sonPid;

    if((shmPort = shmget(PORT_POS_KEY,sizeof(struct port)*SO_PORTI,IPC_CREAT|IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    ports = (struct port*) shmat(shmPort,NULL,0);
    if (ports == (void *) -1){
        fprintf(stderr,"Error assing ports to shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*ports=malloc(sizeof(struct port)*SO_PORTI);*/
    
    ports[0].coord.x = ports[0].coord.y = 0; 
    ports[1].coord.x = ports[1].coord.y = SO_LATO; 
    ports[2].coord.x = 0; 
    ports[2].coord.y = SO_LATO;
    ports[3].coord.x = SO_LATO; 
    ports[3].coord.y = 0;

    /*generating SO_PORTI-4 ports*/
    for (i=4;i<SO_PORTI;i++){  
        do{
            myC=generateRandCoords(); 
        }while(arrContains(ports,myC,i));
        ports[i].coord=myC;
    }
    
    for(i = 0;i < SO_PORTI;i++){
        sprintf(x,"%f", ports[i].coord.x);
        sprintf(y,"%f", ports[i].coord.y);
        switch (sonPid = fork()){
            case -1:
                fprintf(stderr,"Error in fork, %d: %s",errno,strerror(errno));
                return -1;
            case 0:
                if(execlp("./port","./port",NULL) == -1){
                    fprintf(stderr,"Error in execlp port numer %d, %d: %s",i,errno,strerror(errno));
                    return -1;
                }
            break;
            default:
                ports[i].pidPort = sonPid;
                printf("%d %f %f\n",sonPid,ports[i].coord.x,ports[i].coord.y);
            break;
        }
    }

    return 0;
}

/*
Input: void
Output: int
Desc: returns 0 if it has generated all ships correctly, 1 otherwise
*/
int shipGenerator(){
    int i;

    for(i=0;i<SO_NAVI;i++){ 
        switch(fork()){
            case -1:
                fprintf(stderr,"Error in fork , %d: %s \n",errno,strerror(errno));
                return -1;
            case 0: 
                if(execlp("./ship","./ship",NULL) == -1){
                    fprintf(stderr,"Error in execl ship num %d, %d: %s \n",i,errno,strerror(errno));
                    return -1; 
                }
            break;
            default:
            break; 
        }
    }
    return 0;
}


/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/
int printDump(int dSem,struct goods_dump* good_d,struct port_dump* port_d,struct ship_dump* ship_d){
    struct sembuf sops_dump;
    int i;
    int allOffer = 1;
    int allDemand = 1;
    sops_dump.sem_op=-1;
    semop(dSem,&sops_dump,1);

    printf("MERCI \n");
    for(i = 0;i<SO_MERCI;i++){
        printf("Tipo %d:\n",i+1);
        printf("- Merce in porto %d\n",good_d->states[i].goods_in_port);
        printf("- Merce sulle navi %d\n",good_d->states[i].goods_on_ship);
        printf("- Merce scaduta in porto %d\n",good_d->states[i].goods_expired_port);
        printf("- Merce scaduta in nave %d\n",good_d->states[i].goods_expired_ship);
        printf("- Merce ricevuta %d\n",good_d->states[i].goods_delivered);
    }

    printf("\nPORTI\n");
    for(i = 0; i< SO_PORTI; i++){
        printf("Porto %d",i+1);
        printf("- Merce inviata %d\n",port_d->states[i].goods_sended);
        printf("- Merce ricevuta %d\n",port_d->states[i].goods_receved);
        printf("- Merce offerta %d\n",port_d->states[i].goods_offer);
        allOffer = allOffer && port_d->states[i].goods_offer == 0;
        allDemand = allDemand && port_d->states[i].goods_demand == 0;
        printf("- Merce banchine occupate %d\n",port_d->states[i].dock_occuped);
        printf("- Merce banchine totali %d\n",port_d->states[i].dock_total);
    }

    printf("\nNAVI: \n");
    printf("- Navi in porto %d\n",ship_d->ship_in_port);
    printf("- Navi in mare con carico %d\n",ship_d->ship_sea_goods);
    printf("- Navi in mare senza carico %d\n",ship_d->ship_sea_no_goods);

    sops_dump.sem_op=1;
    semop(dSem,&sops_dump,1);

    return allOffer == 1 || allDemand == 1;
}

/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/
void killAllPorts(){
    int i;
    for(i = 0; i< SO_PORTI;i++){
        kill(ports[i].pidPort,SIGTERM);
    }
}

/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/
int deallocateResources(){

    int sySem;
    int shmPort;
    int dSem;
    int shm_dump_goods;
    int shm_dump_port;
    struct port_dump * struct_port_dump;
    struct ship_dump *struct_ship_dump;
    struct goods_dump *struct_goods_dump;

    /*Sy SEMAPHORE */

    if((sySem = semget(SY_KEY,1,0666)) == -1){
        TEST_ERROR;
    }
    if((sySem=semctl(sySem,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }


    /*SHARED MEMORY Ports*/

    if((shmPort = shmget(PORT_POS_KEY,0,0666 )) == -1){
        TEST_ERROR;
    }

    shmdt(ports);
    TEST_ERROR;
    
    if((shmPort = shmctl(shmPort,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }

    /*Sem for dump*/
    if((dSem = semget(DUMP_KEY,1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        TEST_ERROR;
    }

    if((dSem=semctl(dSem,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }

    /*Shared memory for dump of goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_dump),IPC_CREAT |IPC_EXCL|0666 )) == -1){
        TEST_ERROR;
    }
    
    struct_goods_dump=(struct goods_dump*) shmat(shm_dump_goods,NULL,0);
    shmdt(struct_goods_dump);
    TEST_ERROR;
    
    if((shm_dump_goods = shmctl(shm_dump_goods,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }

    /*Shared memory for dump of port*/

    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_dump),IPC_CREAT |IPC_EXCL|0666)) == -1){
        TEST_ERROR;
    }
    struct_port_dump=(struct port_dump*)shmat(shm_dump_port,NULL,0);
    shmdt(struct_port_dump);

    if((shm_dump_port = shmctl(shm_dump_port,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }
    /*Shared memory for dump of ship*/

     if((shm_dump_ship=shmget(SHIP_DUMP_KEY,sizeof(struct ship_dump),IPC_CREAT |IPC_EXCL|0666)) == -1){
        TEST_ERROR;
    }
    struct_ship_dump=(struct ship_dump*)shmat(shm_dump_ship,NULL,0);
    TEST_ERROR;

    shmdt(struct_ship_dump);

    if((struct_ship_dump = shmctl(struct_ship_dump,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }
    
    return 0;

}

