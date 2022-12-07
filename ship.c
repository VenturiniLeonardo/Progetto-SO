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
#include <math.h>  /* for abs of value (double compare function)*/
#include <sys/shm.h>

/*Own libraries or definitions*/
#include "config.h"
#include "struct.h"

/*Function declaration*/
struct coords generateRandCoords();
double distanza(double,double,double,double);
int docking(pid_t);
int getSupply(pid_t);

int main(){
    struct coords ship_coords;
    struct timespec tim;
    int sySem;
    int shmPort;
    struct port *ports;
    struct sembuf sops; 
    int index_min;  
    int i;
    double min_distance;
    double ris_distanza;

    ship_coords=generateRandCoords();


    if((shmPort = shmget(PORT_POS_KEY,0,0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    if (ports == (void *) -1){
        fprintf(stderr,"Error assing ports to shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*Sy Semaphore*/
    if((sySem = semget(getppid(),1,0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 

/*At start Ship goes to nearest port*/
    
    min_distance=SO_LATO*sqrt(2);
    index_min=0;

    for(i=0;i<SO_NAVI;i++){
        ris_distanza=distanza(ports[i].coord.x,ports[i].coord.y,ship_coords.x,ship_coords.y);
        if(ris_distanza<min_distance){
            min_distance=ris_distanza;
            index_min=i;
        }
    }

    tim.tv_sec=0;
    tim.tv_nsec=(min_distance/SO_SPEED);
    if(nanosleep(&tim,NULL)<0){


    }



    /*
    do{








    }while();*/





}


/*Functions definitions*/

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
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
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
double distanza(double x_p,double y_p,double x_n,double y_n){
    return sqrt(pow(x_n-x_p,2)+pow(y_n-y_p,2));
}

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int docking(pid_t pid_port){
        int dockSem;
        struct sembuf sops; 


        /*Semaphore Dock*/
        dockSem = semget(pid_port,1,IPC_CREAT|IPC_EXCL|0666);
        TEST_ERROR;

        sops.sem_num=0;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(dockSem,&sops,1); 
}

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int getSupply(pid_t pid_port){

        key_t supplyKey;
        int semSupply;
        int shmSupply;
        struct sembuf sops; 
        struct good *supply;
        int i;
        int min_exipry_index = SO_MAX_VITA+1;

        /*Get shm semaphore*/
        supplyKey=ftok("/",pid_port);
        semSupply = semget(supplyKey,1,IPC_CREAT|IPC_EXCL|0666);
        TEST_ERROR;

        sops.sem_num=0;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(semSupply,&sops,1); 

        /*Access to supply shm memory*/

        shmSupply=shmget(pid_port,0, IPC_CREAT | IPC_EXCL | 0666);
        supply = (struct good*)shmat(shmSupply,NULL,0);
        for(i=0;i<SO_MERCI;i++){
            if(supply[i].date_expiry < min_exipry_index)
                min_exipry_index = i;
        }


}
