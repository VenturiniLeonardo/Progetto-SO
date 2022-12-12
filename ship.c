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
struct port* near_ports(pid_t,double,int *);
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


    /*At start Ship goes to nearest port*/
    currentPort = nearPort(0,ship_coords);
    printf("Prossimo porto: %d %f %f\n",currentPort->pidPort, currentPort->coord.x,currentPort->coord.y);

    tim.tv_sec=0;
    tim.tv_nsec=(distance(currentPort->coord.x,currentPort->coord.y,ship_coords.x,ship_coords.y)/SO_SPEED);
    if(nanosleep(&tim,NULL)<0){
        TEST_ERROR;
    }

    do{
        printf("%d:   Arrivato al porto %d dopo %ld ns\n",getpid(),currentPort->pidPort,tim.tv_nsec);
        nextPort = duck_access_load(currentPort->pidPort);
        printf("%d:  Prossimo porto %d\n",getpid(),nextPort->pidPort);
        if(nextPort == NULL){
            nextPort = nearPort(currentPort->pidPort,currentPort->coord);
            printf("%d:  Non ho trovato nulla prossimo porto %d\n",getpid(),nextPort->pidPort);
        }
        printf("%d:  Partenza \n",getpid());
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
        if((dockSem = semget(pid_port,1,0666))==-1)
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
        if((dockSem = semget(pid_port,1,0666)) == -1)
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
        int portSize = 0;
        int msgDemand;
        struct port *portNear;
        struct msgDemand demandGood;
        long type;
        int findGood;
        int min_exipry_prev;
        short trovato = 0;
        int min_expry=SO_MAX_VITA+1;
        int flagGood = 0;
        struct port *sendPort;
        struct timespec tim;

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

        printf("%d :: Cerco offerta \n",getpid());
        /*Search for min date expiry in current port*/
        for(i=0;i<SO_MERCI;i++){
            if((shmPort->typeGoods[i][0] == 1 )&&(shmPort->supply[i].date_expiry < min_exipry && shmPort->supply[i].date_expiry > 0)){
                printf("%d:   %d t:%d\n",getpid(),pid_port,shmPort->supply[i].type);
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
            printf("NULL %d\n",getpid());
            return NULL;
        }

       /* printf("%d %d %d\n",shmPort->supply[i].type,shmPort->supply[i].quantity,shmPort->supply[i].date_expiry);*/
        /*Set min exipry to prev min exipry*/
        min_exipry_prev = min_exipry;
        min_exipry = SO_MAX_VITA+1;
        

        do{
            portNear = near_ports(pid_port,SO_SPEED*shmPort->supply[type].date_expiry,&portSize); 
            /*Set for demand in near ports*/
            for(i=0;i<portSize && flagGood == 0;i++){
            
                if((msgDemand = msgget(portNear[i].pidPort,0666)) == -1){
                    fprintf(stderr,"Error messagge queue demand in ship, %d: %s\n",errno,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                printf("Cerco in %d per domanda %d\n",msgDemand,getpid());
                
                /*Find goods*/
                if((findGood = msgrcv(msgDemand,&demandGood,sizeof(int),(type+1),IPC_NOWAIT)) != -1){
                    printf("Presa: %d\n",portSize); 
                    /*if(shmPort->supply[type-1].quantity>=demandGood.quantity){
                        if(demandGood.quantity<=SO_CAPACITY){
                            quantity = demandGood.quantity;
                            shmPort->supply[type-1].quantity-=demandGood.quantity;
                        }else{
                            shmPort->supply[type-1].quantity-=SO_CAPACITY;
                            demandGood.quantity -= SO_CAPACITY;
                            quantity = SO_CAPACITY;
                            msgsnd(msgDemand,&demandGood,sizeof(int),0);
                        }
                    }else{
                        if(shmPort->supply[type-1].quantity<=SO_CAPACITY){
                            shmPort->supply[type-1].date_expiry = -1;
                            shmPort->typeGoods[type-1][0] = 0;
                            demandGood.quantity -= shmPort->supply[type-1].quantity;
                            quantity = shmPort->supply[type-1].quantity;
                            msgsnd(msgDemand,&demandGood,sizeof(int),0);
                        }else{
                            shmPort->supply[type-1].quantity-=SO_CAPACITY;    
                            demandGood.quantity -= SO_CAPACITY;
                            quantity = SO_CAPACITY;
                            msgsnd(msgDemand,&demandGood,sizeof(int),0);                       
                        }
                    }*/
                    printf("Carico \n");
                    tim.tv_sec=0;
                    tim.tv_nsec= quantity/SO_LOADSPEED;
                    if(nanosleep(&tim,NULL)<0){
                        TEST_ERROR;
                    }
                    printf("%d:  Merce caricata %d presso il porto %d\n",getpid(),quantity,pid_port);

                    flagGood = 1;
                    break;
                
                }

            }

            /*Find next min exipry date*/
            for(i=0;i<SO_MERCI && flagGood == 0;i++){
                if((shmPort->typeGoods[i][0] == 1 || shmPort->typeGoods[i][1] == 1 )&&(shmPort->supply[i].date_expiry < min_exipry) ){
                    min_exipry= shmPort->supply[i].date_expiry;
                    type = i;
                }
            }
            /*Set min exipry to prev min exipry*/
            min_exipry_prev = min_exipry;
            min_exipry = SO_MAX_VITA+1;
            
        }while(shmPort->supply[i].date_expiry  == min_exipry && flagGood == 0);
        
        
        /*if(shmPort->supply[min_exipry].quantity > SO_MERCI){
            shmPort->supply[min_exipry].quantity -= SO_MERCI;
        }*/
        sops.sem_num=0;
        sops.sem_op= 1;
        sops.sem_flg=0;
        semop(semSupply,&sops,1);

        if(flagGood){
            return &portNear[i];
        }else{
            return NULL;
        }

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

struct port* near_ports(pid_t currPort,double max_distance,int *length){
    int shmPort;
    struct port *ports;
    struct port *port_return;
    int i;
    int x;
    int y;
    int j=0;
    struct coords currCoord;


    if((shmPort = shmget(PORT_POS_KEY,0,0666))==-1){
        TEST_ERROR;
    }

    ports=(struct port *) shmat(shmPort,NULL,SHM_RDONLY);
    if(ports == (void *) -1)
        TEST_ERROR;
    port_return=malloc(sizeof(struct port));
    currCoord = (getCoordFromPid(currPort));
    for(i=0;i<SO_PORTI;i++){                      
        if((ports[i].pidPort != currPort) && (distance(currCoord.x,currCoord.y,ports[i].coord.x,ports[i].coord.y)<max_distance)){

            port_return[j].coord.x=ports[i].coord.x;
            port_return[j].coord.y=ports[i].coord.y;
            port_return[j].pidPort=ports[i].pidPort;
            port_return=realloc(port_return,sizeof(struct port));
            j++;
        }
    }
    *length = j;

    return port_return;  
}
    


struct port* duck_access_load(pid_t pid_port){
    struct port* ports;
    int dockSem;
    struct sembuf sops; 
    int res_Supply;
    struct port *ret_get_supply;


    docking(pid_port);

    printf("%d:  Effettutato attracco %d\n",getpid(),pid_port);
    ret_get_supply = getSupply(pid_port);    
    
    undocking(pid_port);
    
    printf("%d:  Lascio il molo %d\n",getpid(),pid_port);
    printf("asada%d\n",ret_get_supply->pidPort);
    return ret_get_supply;
}

void duck_access_unload(pid_t pidPort){
    struct timespec tim;
    /*Semaphore Dock*/
    docking(pidPort);
    printf("%d:  Scarico\n",getpid());
    tim.tv_sec=0;
    tim.tv_nsec= quantity/SO_LOADSPEED;
    if(nanosleep(&tim,NULL)<0){
        TEST_ERROR;
    }

    undocking(pidPort);

}
