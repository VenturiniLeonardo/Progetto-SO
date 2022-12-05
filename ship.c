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
#include <math.h>  // for abs of value (double compare function)
#include <sys/shm.h>

//Own libraries or definitions
#include "config.h"
#include "struct.h"

//Function declaration
struct coords generateRandCoords();
double distanza(double,double,double,double);

int main(){
    struct coords ship_coords;
    ship_coords=generateRandCoords();

    int sySem;
    if((sySem = semget(getppid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
 

    int shmPort;
    if((shmPort = shmget(PORT_POS_KEY,0,IPC_CREAT |IPC_EXCL|0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct port *ports;
    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    if (ports == (void *) -1){
        fprintf(stderr,"Error assing ports to shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sembuf sops;   
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 

// At start Ship goes to nearest port;
    int i;
    double min_distance=SO_LATO*sqrt(2);
    int index_min=0;
    for(i=0;i<SO_NAVI;i++){
        double ris_distanza;
        ris_distanza=distanza(ports[i].coord.x,ports[i].coord.y,ship_coords.x,ship_coords.y);
        if(ris_distanza<min_distance){
            min_distance=ris_distanza;
            index_min=i;
        }
    }

    struct timespec time;
    time.tv_sec=0;
    time.tv_nsec=(min_distance/SO_SPEED);
    if(nanosleep(&time,NULL)<0){
        
    }

    do{
        







    }while();

    



}


//Functions definitions
struct coords generateRandCoords(){

    struct coords c;
    //srand(time(0));
    double div = RAND_MAX / SO_LATO;
    c.x = rand() / div;
    div = RAND_MAX / SO_LATO;
    c.y = rand() / div;

    return c;
}

double distanza(double x_p,double y_p,double x_n,double y_n){
    return sqrt(pow(x_n-x_p,2)+pow(y_n-y_p,2));
}

