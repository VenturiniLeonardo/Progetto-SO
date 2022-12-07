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
    int i;

    checkParams();
    /*Semaphore for coordinate*/
    if((sySem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(semctl(sySem,0,SETVAL,SO_NAVI+SO_PORTI+1)<0){
        fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*PORT AND SHIP*/

    portGenerator();
    /*shipGenerator();*/

    elapsedDays=0; /*elapsed days*/
    kill(ports[0].pidPort,SIGINT);

    srand(time(NULL));   
    while(elapsedDays<10){
        nRandPort=(rand()%SO_PORTI);
        /*for(i=0;i<nRandPort;i++){

        }*/
        kill(ports[0].pidPort,SIGINT);
        elapsedDays++;
    }
    /*call dump*/
    deallocateResources();
}

/*Functions definitions*/

/* 
Input: int max, int min
Output: int r random
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
Output: struct coords c
Desc: struct coords c with random x and y coords
*/
struct coords generateRandCoords(){

    struct coords c;
    double div;
    srand(getpid());
    div = RAND_MAX / SO_LATO;
    c.x = rand() / div;
    div = RAND_MAX / SO_LATO;
    c.y = rand() / div;

    return c;
}

/*
Input: port's coord array, new random c coords for port
Output: 1 if is already present, 0 otherwise
Desc: check if an c element is present in a array
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
Output: returns 0 if successful -1 otherwise
Desc: Generate SO_PORTI process ports pass it the generated coordinates
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
                if(execlp("./port","./port",x,y,NULL) == -1){
                    fprintf(stderr,"Error in execlp port numer %d, %d: %s",i,errno,strerror(errno));
                    return -1;
                }
            break;
            default:
                ports[i].pidPort = sonPid;
            break;
        }
    }

    return 0;
}

/*
Input: void
Output: returns 0 if successful -1 otherwise
Desc: Generate SO_NAVI process ships
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
Output: returns 0 if successful -1 otherwise
Desc: deallocate all resources 
*/
int deallocateResources(){
/*SEMAPHORE */
    int sySem;
    int shmPort;
    if((sySem = semget(getpid(),1,0666)) == -1){
        fprintf(stderr,"Error semaphore opened, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    if((sySem = shmctl(sySem,IPC_RMID,NULL)) == -1){
        fprintf(stderr,"Error shared memory deallocation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*SHARED MEMORY*/

    if((shmPort = shmget(PORT_POS_KEY,0,0666 )) == -1){
        fprintf(stderr,"Error shared memory opened, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    if((shmPort = shmctl(shmPort,IPC_RMID,NULL)) == -1){
        fprintf(stderr,"Error shared memory deallocation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

}

