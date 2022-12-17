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
int printDump(int,struct goods_states *,struct port_states *,struct ship_dump *);
void printFinalDump(int,struct goods_states *,struct port_states *,struct ship_dump *);
void killAllPorts();
void updateDateExpiry();
void stopAllShips();
int variableUpdate();

struct port *ports;
pid_t * ships;

/*Master main*/
int main(){
    
    int sySem;
    int sem_dump;
    int shm_dump_goods;
    struct goods_states* struct_goods_dump;
    int shm_dump_port;
    struct port_states* struct_port_dump;
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

    if(variableUpdate()){
        printf("Error set all variable\n");
        return 0;
    }
    ships = (pid_t *)malloc(sizeof(pid_t) * SO_NAVI);
    /*Semaphore for sync*/
    if((sySem = semget(SY_KEY,1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(semctl(sySem,0,SETVAL,SO_NAVI+SO_PORTI+1)<0){
        fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Shared memory for ports*/
    if((shmPort = shmget(PORT_POS_KEY,sizeof(struct port)*SO_PORTI,IPC_CREAT|IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Sem for dump*/
    if((dSem = semget(DUMP_KEY,1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(semctl(dSem,0,SETVAL,1) == -1){
        fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Shared memory for dump of goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_states)*SO_MERCI,IPC_CREAT |IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    struct_goods_dump=(struct goods_states*) shmat(shm_dump_goods,NULL,0);
    TEST_ERROR; 

   
    /*Shared memory for dump of port*/

    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_states)*SO_PORTI,IPC_CREAT |IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    struct_port_dump=(struct port_states*)shmat(shm_dump_port,NULL,0);
    TEST_ERROR;

    /*Shared memory for dump of ship*/

    if((shm_dump_ship=shmget(SHIP_DUMP_KEY,sizeof(struct ship_dump),IPC_CREAT |IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct_ship_dump=(struct ship_dump*)shmat(shm_dump_ship,NULL,0);
    TEST_ERROR;


    struct_ship_dump->ship_sea_goods = 0;
    struct_ship_dump->ship_in_port = 0;
    struct_ship_dump->ship_sea_no_goods = SO_NAVI;

    /*PORT AND SHIP*/

    portGenerator();
    shipGenerator();
    /*Semaphore creation docks*/

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
        sleep(1);
        /*nRandPort=rand()%SO_PORTI; 
        kill(ports[nRandPort].pidPort,SIGALRM);  
        for(i=0;i<nRandPort;i++){ 
            RandPort=rand()%SO_PORTI;
            kill(ports[RandPort].pidPort,SIGALRM);
            
        }*/
        printf("Day %d\n",elapsedDays+1);
        updateDateExpiry();
        if(printDump(dSem,struct_goods_dump,struct_port_dump,struct_ship_dump)){
            printf("Offerta o richiesta pari a zero.... Terminazione\n");
            elapsedDays = SO_DAYS;
        }else{
            elapsedDays++;
        }

    }

    printFinalDump(dSem,struct_goods_dump,struct_port_dump,struct_ship_dump);
    stopAllShips();
    killAllPorts();
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
    srand(time(NULL));
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
    char index_port[100];

    if((shmPort = shmget(PORT_POS_KEY,0,0666 )) == -1){
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

        sprintf(index_port, "%d", i);
        switch (sonPid = fork()){
            case -1:
                fprintf(stderr,"Error in fork, %d: %s",errno,strerror(errno));
                return -1;
            case 0:
                if(execlp("./port","./port",index_port,NULL) == -1){
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
    pid_t sonPid;
    
    for(i=0;i<SO_NAVI;i++){ 
        switch(sonPid=fork()){
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
                ships[i] = sonPid;
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
int printDump(int dSem,struct goods_states* good_d,struct port_states* port_d,struct ship_dump* ship_d){
    struct sembuf sops_dump;
    int i;
    int allOffer = 1;
    int allDemand = 1;
    sops_dump.sem_op=-1;
    semop(dSem,&sops_dump,1);

    printf("MERCI \n");
    for(i = 0;i<SO_MERCI;i++){
        printf("Tipo %d:\n",i+1);
        printf("- Merce in porto %d\n",good_d[i].goods_in_port);
        printf("- Merce sulle navi %d\n",good_d[i].goods_on_ship);
        printf("- Merce scaduta in porto %d\n",good_d[i].goods_expired_port);
        printf("- Merce scaduta in nave %d\n",good_d[i].goods_expired_ship);
        printf("- Merce ricevuta %d\n",good_d[i].goods_delivered);
    }

    printf("\nPORTI\n");
    for(i = 0; i< SO_PORTI; i++){
        printf("Porto %d\n",i+1);
        printf("- Merce inviata %d\n",port_d[i].goods_sended);
        printf("- Merce ricevuta %d\n",port_d[i].goods_receved);
        printf("- Merce offerta %d\n",port_d[i].goods_offer);
        allOffer = allOffer && port_d[i].goods_offer == 0;
        allDemand = allDemand && port_d[i].goods_demand == 0;
        printf("- Banchine occupate %d\n",port_d[i].dock_occuped);
        printf("- Banchine totali %d\n",port_d[i].dock_total);
    }

    printf("\nNAVI: \n");
    printf("- Navi in porto %d\n",ship_d->ship_in_port);
    printf("- Navi in mare con carico %d\n",ship_d->ship_sea_goods);
    printf("- Navi in mare senza carico %d\n",ship_d->ship_sea_no_goods);

    sops_dump.sem_op=1;
    semop(dSem,&sops_dump,1);

    return allOffer == 1 || allDemand == 1;
}


void printFinalDump(int dSem,struct goods_states* good_d,struct port_states* port_d,struct ship_dump* ship_d){
    struct sembuf sops_dump;
    int i;
    int maxSupply = 0;
    int indexMaxSupply = -1;
    int maxDemand = 0;
    int indexMaxDemand = -1;

    sops_dump.sem_op=-1;
    semop(dSem,&sops_dump,1);
    printf("\n\nREPORT FINALE\n");
    printf("MERCI \n");
    for(i = 0;i<SO_MERCI;i++){
        printf("Tipo %d:\n",i+1);
        printf("- Merce in porto %d\n",good_d[i].goods_in_port);
        printf("- Merce sulle navi %d\n",good_d[i].goods_on_ship);
        printf("- Merce scaduta in porto %d\n",good_d[i].goods_expired_port);
        printf("- Merce scaduta in nave %d\n",good_d[i].goods_expired_ship);
        printf("- Merce consegnata %d\n",good_d[i].goods_delivered);
        printf("- Merce presente ad inizio simulazione %d\n",good_d[i].goods_in_port+
                                                            good_d[i].goods_on_ship+
                                                            good_d[i].goods_expired_port+
                                                            good_d[i].goods_expired_ship+
                                                            good_d[i].goods_delivered);

    }

    printf("\nPORTI\n");
    for(i = 0; i< SO_PORTI; i++){
        if(maxSupply < port_d[i].goods_offer){
            maxSupply = port_d[i].goods_offer;
            indexMaxSupply = i;
        }
        if(maxDemand < port_d[i].goods_receved){
            maxDemand = port_d[i].goods_receved;
            indexMaxDemand = i;
        }
        printf("Porto %d\n",i+1);
        printf("- Merce inviata %d\n",port_d[i].goods_sended);
        printf("- Merce ricevuta %d\n",port_d[i].goods_receved);
        printf("- Merce offerta %d\n",port_d[i].goods_offer);
    }


    printf("\nNAVI: \n");
    printf("- Navi occupano una banchina %d\n",ship_d->ship_in_port);
    printf("- Navi in mare con carico %d\n",ship_d->ship_sea_goods);
    printf("- Navi in mare senza carico %d\n",ship_d->ship_sea_no_goods);

    printf("\nRECORD: \n");
    printf("Il porto %d ha offerto più merce -> %d ton\n",indexMaxSupply+1,maxSupply);
    printf("Il porto %d ha richiesto più merce -> %d ton\n",indexMaxDemand+1,maxDemand);


    sops_dump.sem_op=1;
    semop(dSem,&sops_dump,1);

}

/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/

void killAllPorts(){
    int i;
    for(i = 0 ; i< SO_PORTI;i++){
        kill(ports[i].pidPort,SIGTERM);
    }
}

/*
Input: void
Output: int    printf("in %d\n",SO_FILL);
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/

void stopAllShips(){
    int i;
    for(i = 0; i< SO_NAVI;i++){
        kill(ships[i],SIGTERM);
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

/*
Input: void
Output: int
Desc: returns 0 if deallocate all resources, -1 otherwise 
*/

void updateDateExpiry(){
        int i;
    for(i = 0; i< SO_PORTI;i++){
        kill(ports[i].pidPort,SIGUSR1);
    }

    for(i = 0;i<SO_NAVI;i++){
        kill(ships[i],SIGUSR1);
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
    int shm_dump_ship;
    struct port_states * struct_port_dump;
    struct ship_dump *struct_ship_dump;
    struct goods_states *struct_goods_dump;

    free(ships);

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
    if(shmdt(ports) == -1)
        TEST_ERROR;
    
    if((shmPort = shmctl(shmPort,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }

    /*Sem for dump*/
    if((dSem = semget(DUMP_KEY,1,0666)) == -1){
        TEST_ERROR;
    }

    if((dSem=semctl(dSem,0,IPC_RMID)) == -1){
        TEST_ERROR;
    }

    /*Shared memory for dump of goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_states),0666 )) == -1){
        TEST_ERROR;
    }
    
    struct_goods_dump=(struct goods_states*) shmat(shm_dump_goods,NULL,0);
    if(struct_goods_dump == (void *) -1)
        TEST_ERROR;
    if(shmdt(struct_goods_dump)==-1)
        TEST_ERROR;
    
    if((shm_dump_goods = shmctl(shm_dump_goods,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }

    /*Shared memory for dump of port*/

    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_states),0666)) == -1){
        TEST_ERROR;
    }
    struct_port_dump=(struct port_states*)shmat(shm_dump_port,NULL,0);

    if(struct_port_dump == (void *) -1)
        TEST_ERROR;

    if(shmdt(struct_port_dump) == -1)
        TEST_ERROR;

    if((shm_dump_port = shmctl(shm_dump_port,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }
    /*Shared memory for dump of ship*/

    if((shm_dump_ship=shmget(SHIP_DUMP_KEY,sizeof(struct ship_dump),0666)) == -1){
        TEST_ERROR;
    }

    struct_ship_dump=(struct ship_dump*)shmat(shm_dump_ship,NULL,0);
    if(struct_ship_dump == (void *) -1)
        TEST_ERROR;

    if(shmdt(struct_ship_dump) == -1)
        TEST_ERROR;

    if((shm_dump_ship = shmctl(shm_dump_ship,IPC_RMID,NULL)) == -1){
        TEST_ERROR;
    }
    
    return 0;

}

