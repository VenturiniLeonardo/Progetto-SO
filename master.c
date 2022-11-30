//System libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* For portability */
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

//Own libraries or definitions
#include "config.h"
#include "struct.h"

//Function declaration
int portGenerator();

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
}

//Functions definitions
int portGenerator(){

}


