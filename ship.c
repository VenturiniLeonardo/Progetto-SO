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
#include <sys/msg.h>

/*Own libraries or definitions*/
#include "config.h"
#include "struct.h"

/*Function declaration*/
struct coords generateRandCoords();
double distance(double,double,double,double);
int docking(pid_t);
struct port* getSupply(pid_t);
struct port* near_ports(struct coords,double);
struct port* duck_access(int);
struct coords getCoordFromPid(int);
int nearPort();



struct port *ports;
struct coords ship_coords;

int main(){
    struct timespec tim;
    int sySem;
    int shmPort;
    struct sembuf sops; 
    struct port *nextPort;
    int min_distance=SO_LATO*sqrt(2);

    ship_coords=generateRandCoords();


    if((shmPort = shmget(PORT_POS_KEY,0,0666 )) == -1){
        fprintf(stderr,"Error shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    if (ports == (void *) -1){
        fprintf(stderr,"Error assing ports to shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 

    sops.sem_op=0;
    semop(sySem,&sops,1);

    /*At start Ship goes to nearest port*/
    
    index_min = nearPort();

    tim.tv_sec=0;
    tim.tv_nsec=(min_distance/SO_SPEED);
    if(nanosleep(&tim,NULL)<0){
        TEST_ERROR;
    }

    do{
        nextPort = duck_access(index_min);
        if(nextPort == NULL){
            
        }else{
            
        }
        
    }while(1);

}


/*Functions definitions*/

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int nearPort(){

    int min_distance=SO_LATO*sqrt(2);
    int index_min=0;
    int i;
    int ris_distanza;

    for(i=0;i<SO_NAVI;i++){
        ris_distanza=distance(ports[i].coord.x,ports[i].coord.y,ship_coords.x,ship_coords.y);
        if(ris_distanza<min_distance){
            min_distance=ris_distanza;
            index_min=i;
        }
    }   
    return index_min;
}

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
double distance(double x_p,double y_p,double x_n,double y_n){
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
struct port* getSupply(pid_t pid_port){

        key_t supplyKey;
        int semSupply;
        int shmSupply;
        struct sembuf sops; 
        struct shmSinglePort * shmPort;
        int i;
        int min_exipry = SO_MAX_VITA+1;
        int portSize;
        int msgDemand;
        struct port *portNear;
        struct good demandGood;
        short type;
        int findGood;
        int min_exipry_prev;
        struct port* sendPort = NULL;

        /*Get shm semaphore*/
        supplyKey=ftok("./port.c",pid_port);
        semSupply = semget(supplyKey,1,IPC_CREAT|IPC_EXCL|0666);
        TEST_ERROR;

        sops.sem_num=0;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(semSupply,&sops,1); 

        /*Access to supply shm mstruct port[] near_ports()od*)shmat(shmSupply,NULL,0);*/
        shmSupply = shmget(pid_port,0,0666);
        TEST_ERROR;
        shmPort = (struct shmSinglePort *)shmat(shmSupply,NULL,0);


        /*Search for min date expiry in current port*/
        for(i=0;i<SO_MERCI;i++){
            if(shmPort->supply[i].date_expiry < min_exipry && shmPort->supply[i].date_expiry > 0 ){
                min_exipry = shmPort->supply[i].date_expiry;
                type = i;
            }
        }

        /*Set min exipry to prev min exipry*/
        min_exipry_prev = min_exipry;
        min_exipry = SO_MAX_VITA+1;
        do{
            portNear = near_ports(getCoordFromPid(pid_port),SO_SPEED*shmPort->supply[min_exipry_prev].date_expiry);    
            portSize = sizeof(portNear);

            /*Set for demand in near ports*/
            for(i=0;i<portSize;i++){
                if((msgDemand = msgget(portNear[i].pidPort,0666)) == -1){
                    fprintf(stderr,"Error messagge queue demand in ship, %d: %s\n",errno,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                /*Find goods*/
                if((findGood = msgrcv(msgDemand,&demandGood,sizeof(struct good),type,0)) != -1){
                    sendPort = &(portNear[i]);
                    break;
                }

            }

            /*Find next min exipry date*/
            for(i=0;i<SO_MERCI;i++){
                if(shmPort->supply[i].date_expiry >= min_exipry_prev && shmPort->supply[i].date_expiry  < min_exipry && shmPort->supply[i].date_expiry > 0){
                    min_exipry= shmPort->supply[i].date_expiry;
                    type = i;
                }
            }
            /*Set min exipry to prev min exipry*/
            min_exipry_prev = min_exipry;
            min_exipry = SO_MAX_VITA+1;
            
        }while(shmPort->supply[i].date_expiry  == min_exipry);

        
        /*if(shmPort->supply[min_exipry].quantity > SO_MERCI){
            shmPort->supply[min_exipry].quantity -= SO_MERCI;
        }*/

        return sendPort;

}

struct coords getCoordFromPid(int pid){
    int i;
    struct coords c;
    for(i = 0;i < SO_PORTI;i ++){
        if(ports[i].pidPort == pid)
            return ports[i].coord;
    }

    return c;
}

struct port* near_ports(struct coords port_coords,double max_distance){
    int shmPort;
    struct port *ports;
    struct port *port_return;
    int i;
    int x;
    int y;
    int j=0;
    shmPort = shmget(PORT_POS_KEY,0,0666);
    TEST_ERROR;
    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    TEST_ERROR;
    port_return=malloc(sizeof(struct port));
    for(i=0;i<SO_PORTI;i++){
        if(distance(port_coords.x,port_coords.y,ports[i].coord.x,ports[i].coord.y)<max_distance){
            port_return[j].coord.x=ports[i].coord.x;
            port_return[j].coord.y=ports[i].coord.y;
            port_return[j].pidPort=ports[i].pidPort;
            port_return=realloc(port_return,sizeof(struct port));
            j++;
        }
    }
    return port_return;    
}
    


struct port* duck_access(int index_min){
    int shmPort;
    struct port *ports;
    pid_t port_access;
    int dockSem;
    struct sembuf sops; 
    int res_Supply;
    struct port *ret_get_supply;
    shmPort = shmget(PORT_POS_KEY,0,0666);
    TEST_ERROR;
    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    TEST_ERROR;

    port_access=ports[index_min].pidPort;
    dockSem = semget(port_access,1,IPC_CREAT|IPC_EXCL|0666);
    TEST_ERROR;
    sops.sem_op=-1;
    semop(dockSem,&sops,1);
    ret_get_supply = getSupply(port_access);
    sops.sem_op=1;
    semop(dockSem,&sops,1);

    return ret_get_supply;
}