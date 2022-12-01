//System libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> // For portability
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <float.h> // for double compare in arrContains
#include <math.h>  // for abs of value (double compare function)

//Own libraries or definitions
#include "config.h"
#include "struct.h"

//Function declaration
int arrContains(struct port[],struct coords,int );
int portGenerator();
int shipGenerator(int);
int arrContains(struct port [],struct coords ,int );

//Master main
int main(){
    checkParams();
    
    //Semaphore creation
    int sySem;
    if((sySem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0600)) == -1){
        fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if(semctl(sySem,0,SETVAL,SO_NAVI+SO_PORTI+1)<0){
        fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }

    portGenerator();
    sleep(10);
    //shipGenerator(sySem);
}

//Functions definitions

/*
Input: port's coord array, new random c coords for port
Output: 1 if is already present, 0 otherwise
Desc: check if an c element is present in a array
*/
int arrContains(struct port arr[],struct coords c,int arrLen){
    
    int contains=0;
    int i;
    double cX,cY;
    printf("epsilon is %f \n so epsilon-epsilon==0 is %d\n",DBL_EPSILON,((DBL_EPSILON-DBL_EPSILON)<=DBL_EPSILON));
    
    for(i=0;i<=arrLen;i++){
        printf("looking for %f %f in arr[%d]...",c.x,c.y,i);
        cX=(arr[i].coord.x)-c.x;
        cY=(arr[i].coord.y)-c.y;
        if(((fabs(cX))<=DBL_EPSILON) ){ //&&((fabs(cY))<=DBL_EPSILON)){
            contains=1;
            printf("found!");
            break;
        }
        printf("not found in arr[%d] (%f %f) [%f,%f] {%d,%d}\n",i,arr[i].coord.x,arr[i].coord.y,fabs(cX),fabs(cY),((fabs(cX))<=DBL_EPSILON),((fabs(cY))<=DBL_EPSILON));
    }
    return contains;
}

/* 
Input: void
Output: struct coords c
Desc: struct coords c with random x and y coords
*/
struct coords generateRandCoords(){

    struct coords c;
    double range = SO_LATO; 
    double div = RAND_MAX / range;
    c.x=(rand() /div );
    c.y=(rand() /div );
    
    return c;
}

/*
Input: void
Output: returns 0 if successful -1 otherwise
Desc: Generate SO_PORTI process ports pass it the generated coordinates
*/
int portGenerator(){
    
    struct port ports[SO_PORTI];
    ports[0].coord.x = ports[0].coord.y = 0; //coord (0,0)
    ports[1].coord.x = ports[1].coord.y = SO_LATO; //coord (SO_LATO,SO_LATO)
    ports[2].coord.x = 0; //coord(0,SO_LATO)
    ports[2].coord.y = SO_LATO;
    ports[3].coord.x = SO_LATO; //coord(SO_LATO,0)
    ports[3].coord.y = 0;
    
    struct coords myC;
    myC.x=SO_LATO;
    myC.y=0;
    
    int i;
    int j;
    int arrLen;
    struct coords rn;

    for (i=4;i<SO_PORTI;i++){   //generating SO_PORTI-4 ports
        j=0;
        arrLen = (sizeof(ports) / sizeof (struct port)); //getting ports array length for arrContains function
        do{
            j++;
            rn=generateRandCoords(); 
            printf("try %d , rn is %f %f\n",j,rn.x,rn.y);
        }while (arrContains(ports,rn,arrLen));
        ports[i].coord=rn;
    }

    char x[50];
    char y[50];

    for(i = 0;i < SO_PORTI;i++){
        sprintf(x,"%f", ports[i].coord.x);
        sprintf(y,"%f", ports[i].coord.y);
        switch (ports[i].pidPort = fork()){
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
            break;
          
        }
    }

    return 0;
}

/*
Input: semaphore for start coordination
Output: returns 0 if successful -1 otherwise
Desc: Generate SO_NAVI process ships
*/
int shipGenerator(int sySem){
    pid_t fork_ris;
    int i;
    pid_t ship_pid[1];

    struct sembuf sops;
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    

    for(i=0;i<SO_NAVI;i++){     
        switch(ship_pid[i]=fork()){
            case -1:
                fprintf(stderr,"Error in fork , %d: %s \n",errno,strerror(errno));
                return -1;
            case 0:
            /* 
                srand(time(0));
                struct ship;
                ship.coord.x=rand()%SO_LATO;
                ship.coord.y=rand()%SO_LATO;
            */  
                semop(sySem,&sops,1);
                if(execlp("./ship","./ship",NULL) == -1){
                    fprintf(stderr,"Error in execl ship num %d, %d: %s \n",i,errno,strerror(errno));
                    return -1;                                    
                }
                break;
            default:
                break;           
        }
    }
}

