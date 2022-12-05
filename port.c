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

//Own libraries or definitions
#include "config.h"
#include "struct.h"

//Function declaration
int generatorDock();
void generatorSupply();


//Port Main
int main(int argc, char **argv){
    int nDocks = generateDock();
    int currFill = 0;

    //Semaphore creation
    int dockSem;
    if((dockSem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0600)) == -1){
        fprintf(stderr,"Error docks semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if(semctl(dockSem,0,SETVAL,nDocks)<0){
        fprintf(stderr,"Error initializing docks semaphore, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }


    /*Sy Semaphore*/
    int sySem;
    if((sySem = semget(getppid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
        fprintf(stderr,"Error sy semaphore creation, %d: %s\n",errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    struct sembuf sops;
    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sySem,&sops,1); 


    


}


//Functions definitions

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
int generatorDock(){
    srand(time(NULL));
    return rand()%SO_BANCHINE;
}

/*
Input: void
Output: int
Desc: return random int between 1 and SO_BANCHINE
*/
void generatorSupply(){
    
}
