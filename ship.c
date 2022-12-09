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
int undocking(pid_t);
struct port* getSupply(pid_t);
struct port* near_ports(struct coords,double);
struct port* duck_access_load(pid_t);
void duck_access_unload(pid_t);
struct coords getCoordFromPid(int);
struct port* nearPort(pid_t ,struct coords);
int quantity;


struct port *ports;

int main(){
    struct timespec tim;
    int sySem;
    int shmPort;
    struct sembuf sops; 
    struct port* nextPort;
    struct port* currentPort;
    struct coords ship_coords;
    int i = 0;

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
    currentPort = nearPort(0,ship_coords);
    printf("Prossimo porto: %d %f %f\n",currentPort->pidPort, currentPort->coord.x,currentPort->coord.y);

    tim.tv_sec=0;
    tim.tv_nsec=(distance(currentPort->coord.x,currentPort->coord.y,ship_coords.x,ship_coords.y)/SO_SPEED);
    if(nanosleep(&tim,NULL)<0){
        TEST_ERROR;
    }

    do{
        printf("Arrivato al porto %d dopo %ld ns\n",currentPort->pidPort,tim.tv_nsec);
        nextPort = duck_access_load(currentPort->pidPort);
        printf("Prossimo porto\n");
        if(nextPort == NULL){
            nextPort = nearPort(currentPort->pidPort,currentPort->coord);
            printf("Non ho trovato nulla prossimo porto %d\n",nextPort->pidPort);
        }
        printf("Partenza \n");
        tim.tv_sec=0;
        tim.tv_nsec=(distance(nextPort->coord.x,nextPort->coord.y,currentPort->coord.x,currentPort->coord.y)/SO_SPEED);
        currentPort = nextPort;
        if(nanosleep(&tim,NULL)<0){
            TEST_ERROR;
        }
        
        duck_access_unload(currentPort->pidPort);
        i++;
    }while(i<SO_DAYS);

}


/*Functions definitions*/

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
struct port* nearPort(pid_t pidPort,struct coords ship_coords){

    int min_distance=SO_LATO*sqrt(2);
    int i;
    int ris_distanza;
    int index_min;

    for(i=0;i<SO_PORTI;i++){
        ris_distanza=distance(ports[i].coord.x,ports[i].coord.y,ship_coords.x,ship_coords.y);
        if(ris_distanza<min_distance && ports[i].pidPort != pidPort){
            min_distance=ris_distanza;
            index_min = i;
        }
    }   

    
    return &(ports[index_min]);
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
        dockSem = semget(pid_port,1,0666);
        TEST_ERROR;

        sops.sem_num=0;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(dockSem,&sops,1); 

        return 0;
}

int undocking(pid_t pid_port){
            int dockSem;
        struct sembuf sops; 


        /*Semaphore Dock*/
        dockSem = semget(pid_port,1,0666);
        TEST_ERROR;

        sops.sem_op= 1;
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
        short trovato = 0;

        /*Get shm semaphore*/
        supplyKey=ftok("./port.c",pid_port);
        if((semSupply = semget(supplyKey,1,0666)) == -1){
            TEST_ERROR;
        }
        
        sops.sem_num=0;
        sops.sem_op = -1;
        sops.sem_flg=0;
        semop(semSupply,&sops,1); 
        

        /*Access to supply shm mstruct port[] near_ports()od*)shmat(shmSupply,NULL,0);*/
        if((shmSupply = shmget(pid_port,0,0666)) == -1){
            TEST_ERROR;
        }

        shmPort = (struct shmSinglePort *)shmat(shmSupply,NULL,0);
        if(shmPort == (void *) -1)
            TEST_ERROR;

        printf("Cerco offerta\n");
        /*Search for min date expiry in current port*/
        for(i=0;i<SO_MERCI;i++){
            if((shmPort->typeGoods[i][0] == 1 )&&(shmPort->supply[i].date_expiry < min_exipry && shmPort->supply[i].date_expiry > 0) ){
                trovato = 1;
                min_exipry = shmPort->supply[i].date_expiry;
                type = i;
            }
        }

        /*Se non trovo nulla esco subito*/
        if(!trovato){
            sops.sem_num=0;
            sops.sem_op = 1;
            sops.sem_flg=0;
            semop(semSupply,&sops,1); 
            return sendPort;
        }

       /* printf("%d %d %d\n",shmPort->supply[i].type,shmPort->supply[i].quantity,shmPort->supply[i].date_expiry);*/
        /*Set min exipry to prev min exipry*/
        min_exipry_prev = min_exipry;
        min_exipry = SO_MAX_VITA+1;

        do{
            portNear = near_ports(getCoordFromPid(pid_port),SO_SPEED*shmPort->supply[min_exipry_prev].date_expiry);    
            portSize = sizeof(portNear)/sizeof(struct port);
            printf("%d\n",portSize);
            /*Set for demand in near ports*/
            for(i=0;i<portSize;i++){
                printf("Cerco domanda %d\n",portNear[i].pidPort);
                if((msgDemand = msgget(portNear[i].pidPort,0666)) == -1){
                    fprintf(stderr,"Error messagge queue demand in ship, %d: %s\n",errno,strerror(errno));
                    exit(EXIT_FAILURE);
                }

                /*Find goods*/
                if((findGood = msgrcv(msgDemand,&demandGood,sizeof(struct good),type,0)) != -1){
                    sendPort = &(portNear[i]);
                    if(shmPort->supply[type].quantity>=demandGood.quantity){
                        if(demandGood.quantity<=SO_CAPACITY){
                            shmPort->supply[type].quantity-=demandGood.quantity;
                        }else{
                            shmPort->supply[type].quantity-=SO_CAPACITY;
                            demandGood.quantity -= SO_CAPACITY;
                            msgsnd(msgDemand,&demandGood,sizeof(struct good),0);
                        }
                    }else{
                        if(shmPort->supply[type].quantity<=SO_CAPACITY){
                            shmPort->supply[type].date_expiry = -1;
                            shmPort->typeGoods[type][0] = 0;
                            demandGood.quantity -= shmPort->supply[type].quantity;
                            msgsnd(msgDemand,&demandGood,sizeof(struct good),0);
                        }
                        else{
                            shmPort->supply[type].quantity-=SO_CAPACITY;    
                            demandGood.quantity -= SO_CAPACITY;
                            msgsnd(msgDemand,&demandGood,sizeof(struct good),0);                       
                        }

                    printf("Found goods\n");
                    break;
                }

            }

            /*Find next min exipry date*/
            for(i=0;i<SO_MERCI;i++){
                if((shmPort->typeGoods[i][0] == 1 || shmPort->typeGoods[i][1] == 1 )&&(shmPort->supply[i].date_expiry < min_exipry) ){
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
        sops.sem_num=0;
        sops.sem_op= 1;
        sops.sem_flg=0;
        semop(semSupply,&sops,1);
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
    printf("near_ports in\n");
    if((shmPort = shmget(PORT_POS_KEY,0,0666))==-1){
        TEST_ERROR;
    }

    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    if(ports == (void *) -1)
        TEST_ERROR;

    port_return=malloc(sizeof(struct port));
    for(i=0;i<SO_PORTI;i++){
        if(distance(port_coords.x,port_coords.y,ports[i].coord.x,ports[i].coord.y)<max_distance){
            port_return[j].coord.x=ports[i].coord.x;
            port_return[j].coord.y=ports[i].coord.y;
            port_return[j].pidPort=ports[i].pidPort;
            port_return=realloc(port_return,sizeof(struct port));
            j++;
            printf("Indice array porti vicini: %d\n",i);
        }
        printf("NUm port vicini:%d\n",j);
    }
    return port_return;    
}
    


struct port* duck_access_load(pid_t pid_port){
    struct port* ports;
    int dockSem;
    struct sembuf sops; 
    int res_Supply;
    struct port *ret_get_supply;


    docking(pid_port);

    printf("Effettutato attracco %d\n",pid_port);
    ret_get_supply = getSupply(pid_port);    
    
    undocking(pid_port);
    
    printf("Lascio il molo %d\n",pid_port);
    return ret_get_supply;
}

void duck_access_unload(pid_t pidPort){
    struct timespec tim;
    /*Semaphore Dock*/
    docking(pidPort);

    tim.tv_sec=0;
    tim.tv_nsec= quantity/SO_LOADSPEED;
    if(nanosleep(&tim,NULL)<0){
        TEST_ERROR;
    }

    undocking(pidPort);

}
