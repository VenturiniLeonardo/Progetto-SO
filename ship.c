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
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>

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
struct port* dock_access_load(pid_t);
void dock_access_unload(pid_t);
struct coords getCoordFromPid(int);
struct port* nearPort(pid_t,pid_t ,struct coords);
int get_index_from_pid(pid_t);
void deallocateResources();
int getMaxExpiryDate(struct shmSinglePort[]);
void reloadExpiryDate();
int variableUpdate();
void storm_sleep();
int getPositionByShipPid();
void swell();


int dumpSem;
struct good goods_on; 
struct shmSinglePort * shmPort;
struct ship_condition * ships;
struct port *ports;

struct ship_dump *ship_d;
struct goods_states* good_d;
struct port_states* port_d;

int time_expry_on;


/*Handler*/
void signalHandler(int signal){
    switch(signal){
        case SIGUSR2:
            storm_sleep();
        break;
        case SIGUSR1:
            reloadExpiryDate();
        break;
        case SIGTERM:
            deallocateResources();
            exit(EXIT_SUCCESS);
        break;
        case SIGPROF:
            swell();
        break;
    }
}

int main(){
    int shm_ports;
    struct timespec req;
    struct timespec rem ;
    int sySem;
    int shmPort;
    struct sembuf sops; 
    struct port* nextPort;
    struct port* currentPort;
    struct port* prevPort;
    struct coords ship_coords;
    struct sigaction sa;
    sigset_t mask;
    int i = 0;
    int shm_dump_port;
    int shm_dump_ship;
    int shm_dump_goods; 
    double distanza;
    int shmShip;

    if(variableUpdate()){
        printf("Error set all variable\n");
        return 0;
    }

    ship_coords=generateRandCoords();
    
    if((shm_ports = shmget(PORT_POS_KEY,0,0666)) == -1){
        fprintf(stderr,"Error shared memory, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    ports= (struct port *) shmat(shm_ports,NULL,0);
    if (ports == (void *) -1){
        fprintf(stderr,"Error shared memory ports in ship, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*shm for ships attached to a port */
    if((shmShip = shmget(SHIP_POS_KEY,0,0666)) == -1){
        TEST_ERROR; 
    }
    ships =shmat(shmShip,NULL,0);
    
    /*shm_mem for dump_port */
    if((shm_dump_port=shmget(PORT_DUMP_KEY,sizeof(struct port_states),0666))==-1)
        TEST_ERROR;
    port_d=(struct port_states*)shmat(shm_dump_port,NULL,0);
    TEST_ERROR;


    /*shm_mem for dump_ship*/
    if((shm_dump_ship=shmget(SHIP_DUMP_KEY,sizeof(struct ship_dump),0666))==-1)
        TEST_ERROR;
    ship_d=(struct ship_dump*)shmat(shm_dump_ship,NULL,0);
    TEST_ERROR;

    /*Dump  for goods*/
    if((shm_dump_goods = shmget(GOODS_DUMP_KEY,sizeof(struct goods_states),0666 )) == -1){
        fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    good_d=(struct goods_states*)shmat(shm_dump_goods,NULL,0);


    /*Sem for dump*/
    if((dumpSem = semget(DUMP_KEY,1,0666)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    /*Sy Semaphore*/
    if((sySem = semget(SY_KEY,1,0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    bzero(&sa, sizeof(sa));
    sa.sa_handler = signalHandler;
    sa.sa_flags=SA_RESTART||SA_NODEFER;
    sigaction(SIGUSR2,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);
    sigaction(SIGTSTP,&sa,NULL);
    sigemptyset(&mask);

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1);

    sops.sem_op=0;
    semop(sySem,&sops,1);

    
    /*At start Ship goes to nearest port*/
    currentPort = prevPort = nearPort(0,0,ship_coords);
    printf("Prossimo porto: %d %f %f\n",currentPort->pidPort, currentPort->coord.x,currentPort->coord.y);
    distanza=distance(currentPort->coord.x,currentPort->coord.y,ship_coords.x,ship_coords.y)/SO_SPEED;
    rem.tv_sec=0;
    rem.tv_nsec=0;
    req.tv_sec=(int)distanza;
    req.tv_nsec=(distanza-(int)distanza)*10000000;
    while(nanosleep(&req,&rem)<0){
        if(errno!=EINTR){
            TEST_ERROR;
        }else{
            req.tv_sec=rem.tv_sec;
            req.tv_nsec=rem.tv_nsec;
        }
    }

    do{
        printf("%d:   Arrivato al porto %d dopo %ld ns\n",getpid(),currentPort->pidPort,req.tv_nsec);
        nextPort = dock_access_load(currentPort->pidPort);
        if(nextPort->pidPort == 0){
            nextPort = nearPort(prevPort->pidPort,currentPort->pidPort,currentPort->coord);
            printf("%d:  Non ho trovato nulla prossimo porto %d\n",getpid(),nextPort->pidPort);
            printf("%d:  Prossimo porto %d\n",getpid(),nextPort->pidPort);
            printf("%d:  Partenza \n",getpid());
            distanza=distance(nextPort->coord.x,nextPort->coord.y,currentPort->coord.x,currentPort->coord.y)/SO_SPEED;
            req.tv_sec=(int)distanza;
            req.tv_nsec=(distanza-(int)distanza)*1000000000;
            prevPort = currentPort;
            currentPort = nextPort;
            while(nanosleep(&req,&rem)<0){
                if(errno!=EINTR){
                    TEST_ERROR;
                }else{
                    req.tv_sec=rem.tv_sec;
                    req.tv_nsec=rem.tv_nsec;
                }
            }
        }else{
            printf("%d:  Prossimo porto %d\n",getpid(),nextPort->pidPort);
            printf("%d:  Partenza \n",getpid());
            distanza=distance(nextPort->coord.x,nextPort->coord.y,currentPort->coord.x,currentPort->coord.y)/SO_SPEED;
            req.tv_sec=(int)distanza;
            req.tv_nsec=(distanza-(int)distanza)*1000000000;
            
            while(nanosleep(&req,&rem)<0){
                if(errno!=EINTR){
                    TEST_ERROR;
                }else{
                    req.tv_sec=rem.tv_sec;
                    req.tv_nsec=rem.tv_nsec;
                }
            }

            prevPort = currentPort;
            currentPort =nextPort;
            if(goods_on.date_expiry>0)
                dock_access_unload(currentPort->pidPort);
            }
    }while(1);

    return 0;

}


/*Functions definitions*/

/*
Input: pid_t, pid_t, struct coords
Output: struct port*
Desc: return struct port pointer to the nearest port with different pid than current and previous
*/
struct port* nearPort(pid_t prevPort,pid_t pidPort,struct coords ship_coords){

    int min_distance=SO_LATO*sqrt(2);
    int i;
    int ris_distanza;
    int index_min;

    for(i=0;i<SO_PORTI;i++){
        ris_distanza=distance(ports[i].coord.x,ports[i].coord.y,ship_coords.x,ship_coords.y);
        if(ris_distanza<min_distance && ports[i].pidPort != pidPort && ports[i].pidPort != prevPort){
            min_distance=ris_distanza;
            index_min = i;
        }
    }   

        
    return &(ports[index_min]);
}

/*
Input: void
Output: struct coords
Desc: return random coordinates between 0 and SO_LATO
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
Input: double,double,double,double
Output: double
Desc: return distance between 2 points 
*/
double distance(double x_p,double y_p,double x_n,double y_n){
    return sqrt(pow(x_n-x_p,2)+pow(y_n-y_p,2));
}

/*
Input: pid_t
Output: int
Desc: return 0 if the docking has gone abuon end
*/
int docking(pid_t pid_port){
        int dockSem;
        struct sembuf sops; 
        

        /*Semaphore Dock*/
        if((dockSem = semget(pid_port,1,0666))==-1){
            TEST_ERROR;
        }
        sops.sem_num=0;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(dockSem,&sops,1);

        ships[getPositionByShipPid()].port = pid_port;


        return 0;
}

/*
Input: pid_t
Output: int
Desc: returns 0 if the uncoupling has gone to an end
*/
int undocking(pid_t pid_port){
        int dockSem;
        struct sembuf sops; 

        /*Deleting pid from Port shm */


        /*Semaphore Dock*/
        if((dockSem = semget(pid_port,1,0666)) == -1){
            TEST_ERROR;
        }
        sops.sem_num=0;
        sops.sem_op=1;
        sops.sem_flg=0;
        semop(dockSem,&sops,1);
 

    return 0;
}

/*
Input: struct good []
Output: int
Desc: return the maximum expiration date of a supply array
*/
int getMaxExpiryDate(struct shmSinglePort shmPort[]){
    int i;
    int maxDate = 0;
    for(i = 0;i<SO_MERCI;i++){
        if(shmPort[i].supply.date_expiry >= maxDate)
            maxDate = shmPort[i].supply.date_expiry;
    }
    return maxDate;
}


/*
Input: pid_t
Output: struct port*
Desc: returns a struct port pointer containing the next port or an empty port if it finds nothing
*/
struct port* getSupply(pid_t pid_port){
        key_t supplyKey;
        int semSupply;
        int shmSupply;
        struct sembuf sops; 
        int i;
        int min_exipry = SO_MAX_VITA+1;
        int portSize = 0;
        int msgDemand;
        struct port *portNear;
        struct msgDemand demandGood;
        int type;
        int findGood;
        int min_exipry_prev;
        short trovato = 0;
        int min_expry=SO_MAX_VITA+1;
        int flagGood = 0;
        struct port* sendPort;
        struct timespec req;
        struct timespec rem;
        struct sembuf sops_dump;
        double time_nanosleep;

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
        if(shmPort == (void *) -1){
            TEST_ERROR;
        }
        /*printf("%d : Cerco offerta \n",getpid());*/
        /*Search for min date expiry in current port*/
        for(i=0;i<SO_MERCI;i++){
            if((shmPort[i].supplyGoods == 1 )&&(shmPort[i].supply.date_expiry < min_exipry && shmPort[i].supply.date_expiry > 0)){
                trovato = 1;
                min_exipry = shmPort[i].supply.date_expiry;
                type = i;

            }
        }
        /*Se non trovo nulla esco subito*/
        if(!trovato){
            
            sops.sem_num=0;
            sops.sem_op = 1;
            sops.sem_flg=0;
            semop(semSupply,&sops,1);
            
            sendPort = (struct port*)malloc(sizeof(struct port));
            sendPort->coord.x = -1;
            sendPort->coord.y = -1;
            sendPort->pidPort = 0;
            shmdt(shmPort);
            return sendPort;
        }

       /* printf("%d %d %d\n",shmPort->supply[i].type,shmPort->supply[i].quantity,shmPort->supply[i].date_expiry);*/
        /*Set min exipry to prev min exipry*/
        min_exipry_prev = min_exipry;
        min_exipry = SO_MAX_VITA+1;
        
        do{
            portNear = near_ports(pid_port,SO_SPEED*shmPort[type].supply.date_expiry,&portSize); 
            /*Set for demand in near ports*/
            for(i=0;i<portSize && flagGood == 0;i++){
            
                if((msgDemand = msgget(portNear[i].pidPort,0666)) == -1){
                    fprintf(stderr,"Error messagge queue demand in ship, %d: %s\n",errno,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                /*printf("Ricerca per domanda %d\n",getpid());*/
                
                /*Find goods*/
                if((findGood = msgrcv(msgDemand,&demandGood,sizeof(int),(type+1),IPC_NOWAIT)) != -1){
                    /*time_expry_on=shmPort->supply[type].date_expry*/
                    printf("%d   -> s: %d d: %d\n",getpid(),shmPort[type].supply.quantity,demandGood.quantity);
                    if(shmPort[type].supply.quantity>=demandGood.quantity){
                        if(demandGood.quantity<=SO_CAPACITY){
                            goods_on.quantity = demandGood.quantity;
                            shmPort[type].supply.quantity-=demandGood.quantity;
                        }else{
                            shmPort[type].supply.quantity-=SO_CAPACITY;
                            demandGood.quantity -= SO_CAPACITY; 
                            goods_on.quantity = SO_CAPACITY;
                            msgsnd(msgDemand,&demandGood,sizeof(int),0);
                        }
                    }else{
                        if(shmPort[type].supply.quantity<=SO_CAPACITY){
                            shmPort[type].supply.date_expiry = -1;
                            shmPort[type].supplyGoods = 0;
                            demandGood.quantity -= shmPort[type].supply.quantity;
                            goods_on.quantity = shmPort[type].supply.quantity;
                            msgsnd(msgDemand,&demandGood,sizeof(int),0);
                        }else{
                            shmPort[type].supply.quantity-=SO_CAPACITY;    
                            demandGood.quantity -= SO_CAPACITY;
                            goods_on.quantity = SO_CAPACITY;
                            msgsnd(msgDemand,&demandGood,sizeof(int),0);                       
                        }
                    }

                    goods_on.type=type+1;
                    goods_on.date_expiry=shmPort[type].supply.date_expiry;
                    
                    sops_dump.sem_op=-1;
                    semop(dumpSem,&sops_dump,1);
        

                    /*Dump ports*/ 
                    port_d[get_index_from_pid(pid_port)].goods_sended+=goods_on.quantity; 
                    port_d[get_index_from_pid(pid_port)].goods_offer-=goods_on.quantity; 
                    sops_dump.sem_op=1;
                    semop(dumpSem,&sops_dump,1);
                    

                    time_nanosleep=goods_on.quantity/SO_LOADSPEED;
                    req.tv_sec=(int)time_nanosleep;
                    req.tv_nsec= (time_nanosleep-(int)time_nanosleep)*1000000000;
                    /*Time load*/
                    while(nanosleep(&req,&rem)<0){
                        if(errno!=EINTR){
                            TEST_ERROR;
                    }else{
                        req.tv_sec=rem.tv_sec;
                        req.tv_nsec=rem.tv_nsec;
                    }
                    }
                   
                    printf("%d:  Merce caricata %d presso il porto %d\n",getpid(),goods_on.quantity,pid_port);

                    flagGood = 1;
                    break;
                
                }

            }
            
            if(!flagGood){
                /*Find next min exipry date*/
                for(i=0;i<SO_MERCI;i++){
                    
                    if((shmPort[i].supplyGoods == 1)&&(shmPort[i].supply.date_expiry > min_exipry_prev && shmPort[i].supply.date_expiry < min_exipry) ){
                        min_exipry= shmPort[i].supply.date_expiry;
                        type = i;
                    }
                }
                /*Set min exipry to prev min exipry*/
                min_exipry_prev = min_exipry;
                min_exipry = SO_MAX_VITA+1;
            }

            
        }while(getMaxExpiryDate(shmPort)  == min_exipry_prev && flagGood == 0);
        
        sops.sem_num=0;
        sops.sem_op= 1;
        sops.sem_flg=0;
        semop(semSupply,&sops,1);



        if(flagGood){
            shmdt(shmPort);
            return &portNear[i];
        }else{
            sendPort = (struct port*)malloc(sizeof(struct port));
            sendPort->coord.x = -1;
            sendPort->coord.y = -1;
            sendPort->pidPort = 0;
            shmdt(shmPort);
            return sendPort;
        }

}
/*
Input: int
Output: struct coords
Desc: returns struct coords from port pid
*/
struct coords getCoordFromPid(int pid){
    int i;
    struct coords c;
    
    for(i = 0;i < SO_PORTI;i ++){
        if(ports[i].pidPort == pid)
            return ports[i].coord;
    }

    return c;
}

/*
Input: pid_t, double, int
Output: struct port*
Desc: returns an array containing the ports included in the max_distance and updates the int length
*/

struct port* near_ports(pid_t currPort,double max_distance,int *length){
    struct port *port_return;
    int i;
    int x;
    int y;
    int j=0;
    struct coords currCoord;

    port_return=malloc(sizeof(struct port));
    currCoord = (getCoordFromPid(currPort));

    for(i=0;i<SO_PORTI;i++){  
        if((ports[i].pidPort != currPort) && (distance(currCoord.x,currCoord.y,ports[i].coord.x,ports[i].coord.y)<max_distance)){
            port_return[j].coord.y=ports[i].coord.y;
            port_return[j].pidPort=ports[i].pidPort;
                 
            port_return=realloc(port_return,sizeof(struct port));
            j++;
        }
    }
    *length = j;

    return port_return;  
}        
      
    

/*
Input: pid_t
Output: struct port*
Desc: returns a struct port pointer containing the next port or an empty port if it finds nothing - operations docking and undocking
*/
struct port* dock_access_load(pid_t pid_port){
    struct port* ports;
    int dockSem;
    struct sembuf sops; 
    int res_Supply;
    struct port *ret_get_supply;
    struct sembuf sops_dump;

    docking(pid_port);

    sops_dump.sem_op=-1;
    semop(dumpSem,&sops_dump,1);

    /*Dump ships*/
    ship_d->ship_in_port += 1;
    ship_d->ship_sea_no_goods -= 1;
    /*Dump ports*/
    port_d[get_index_from_pid(pid_port)].dock_occuped++;
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);     

    
    printf("%d:  Effettutato attracco %d\n",getpid(),pid_port);
    ret_get_supply = getSupply(pid_port);    
    
    undocking(pid_port);
    sops_dump.sem_op=-1;  
    semop(dumpSem,&sops_dump,1);

    good_d[goods_on.type-1].goods_on_ship += goods_on.quantity;
    good_d[goods_on.type-1].goods_in_port -= goods_on.quantity;
    /*Dump ships*/
    ship_d->ship_in_port -= 1;
    if(ret_get_supply->pidPort == 0)
        ship_d->ship_sea_no_goods += 1;
    else
        ship_d->ship_sea_goods += 1;
    /*Dump ports*/
    port_d[get_index_from_pid(pid_port)].dock_occuped--;
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);

    
    
    printf("%d:  Lascio il molo %d\n",getpid(),pid_port);
    return ret_get_supply;
}

/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
void dock_access_unload(pid_t pidPort){
    struct timespec req;
    struct timespec rem;
    struct sembuf sops_dump;
    double time_nanosleep;

    /*Semaphore Dock*/
    docking(pidPort);
    
    time_nanosleep=goods_on.quantity/SO_LOADSPEED;
    req.tv_sec=(int)time_nanosleep;
    req.tv_nsec= (time_nanosleep-(int)time_nanosleep)*1000000000;
    while(nanosleep(&req,&rem)<0){
        if(errno!=EINTR){
            TEST_ERROR;
        }else{
            req.tv_sec=rem.tv_sec;
            req.tv_nsec=rem.tv_nsec;
        }
    }
    
    sops_dump.sem_op=-1;
    semop(dumpSem,&sops_dump,1);
    printf("%d scarico merce\n",getpid());
    /*Dump ships*/  
    ship_d->ship_in_port += 1;
    ship_d->ship_sea_goods -= 1;
    /*Dump goods*/
    good_d[goods_on.type-1].goods_delivered += goods_on.quantity;
    good_d[goods_on.type-1].goods_on_ship -= goods_on.quantity;
    /*Dump ports*/
    port_d[get_index_from_pid(pidPort)].goods_receved +=goods_on.quantity;
    port_d[get_index_from_pid(pidPort)].goods_demand -=goods_on.quantity;
    /*Dump ports*/
    port_d[get_index_from_pid(pidPort)].dock_occuped++;
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);
    
    undocking(pidPort);
    
    sops_dump.sem_op=-1;
    semop(dumpSem,&sops_dump,1);
    /*Dump ships*/
    ship_d->ship_in_port -= 1;
    ship_d->ship_sea_no_goods += 1;
    /*Dump ports*/
    port_d[get_index_from_pid(pidPort)].dock_occuped--;
    sops_dump.sem_op=1;
    semop(dumpSem,&sops_dump,1);
    
}

/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
int get_index_from_pid(pid_t pidPort){
    int i;
    for(i=0;i<SO_PORTI;i++){
        if(ports[i].pidPort==pidPort)
            return i;
    }
    return -1;
}

/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
int getPositionByShipPid(){
    int i;
    for(i = 0; i<SO_NAVI;i++)
        if(ships[i].ship == getpid())
            return i;

    return -1;   
}

/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
void storm_sleep(){
    struct timespec req;
    struct timespec rem;
    double time_storm=SO_STORM_DURATION/24;
    rem.tv_sec=0;
    rem.tv_nsec=0;
    req.tv_sec=(int)time_storm;
    req.tv_nsec=(time_storm-(int)time_storm)*1000000000;
    while(nanosleep(&req,&rem)<0){
        if(errno!=EINTR){
            TEST_ERROR;
        }else{
            req.tv_sec=rem.tv_sec;
            req.tv_nsec=rem.tv_nsec;
        }
    }
}


/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
void swell(){
    struct timespec req;
    struct timespec rem;
    double time_swell=SO_SWELL_DURATION/24;
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
}
/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
void deallocateResources(){
    shmdt(port_d);
    shmdt(ports);
    shmdt(ship_d);
    shmdt(good_d);
    shmdt(shmPort);
    shmdt(ships);
}

/*
Input: pid_ta: ogni giorno colpisce casualmente
Output: void
Desc: allows access to the loading dock
*/
void reloadExpiryDate(){
    goods_on.date_expiry-=1;
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
        if(strcmp(variable,"SO_STORM_DURATION") == 0){
            SO_STORM_DURATION = atoi(value);
        }
        if(strcmp(variable,"SO_SWELL_DURATION") == 0){
            SO_SWELL_DURATION = atoi(value);
        }
        if(strcmp(variable,"SO_MEALSTROM") == 0){
            SO_MEALSTROM = atoi(value);
        }    

    }

	fclose(f);
    return 0;
}
