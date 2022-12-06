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
#include <math.h> // for abs of value (double compare function)
#include <sys/shm.h>
#include <signal.h>

//Own libraries or definitions
#include "config.h"
#include "struct.h"

//Function declaration
int arrContains(struct port[],struct coords,int );
int portGenerator();
int shipGenerator();
int arrContains(struct port [],struct coords ,int );
int deallocateResources();
int genRandInt(int,int);

//Master main
int main(){
checkParams();
//Semaphore for coordinate
int sySem;
if((sySem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
if(semctl(sySem,0,SETVAL,SO_NAVI+SO_PORTI+1)<0){
fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}

portGenerator();
shipGenerator();

//Sem for dump

int sem_dump;

if((sySem = semget(DUMP_KEY,3,IPC_CREAT|IPC_EXCL|0666)) == -1){
fprintf(stderr,"Error semaphore creation, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
if(semctl(sySem,0,SETVAL,1)<0){
fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
if(semctl(sySem,1,SETVAL,1)<0){
fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
if(semctl(sySem,2,SETVAL,1)<0){
fprintf(stderr,"Error initializing semaphore, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}

//Shared memory for dump of goods

int shm_dump_goods;
if((shm_dump_goods = shmget(GOODS_DUMP_KEY,0,IPC_CREAT |IPC_EXCL|0666 )) == -1){
fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
struct goods_dump* struct_goods_dump;
struct_goods_dump=(struct goods_dump*) shmat(shm_dump_goods,NULL,0);
TEST_ERROR;
//Shared memory for dump of port

int shm_dump_port;
shm_dump_port=shmget(PORT_DUMP_KEY,0,IPC_CREAT |IPC_EXCL|0666);
TEST_ERROR;
struct port_dump* struct_port_dump;
struct_port_dump=(struct port_dump*)shmat(shm_dump_port,NULL,0);
TEST_ERROR;

//Shared memory for dump of ship

int shm_dump_ship;
shm_dump_ship=shmget(SHIP_DUMP_KEY,0,IPC_CREAT |IPC_EXCL|0666);
TEST_ERROR;
struct ship_dump* struct_ship_dump;
struct_ship_dump=(struct ship_dump*)shmat(shm_ship_dump,NULL,0);
TEST_ERROR;

//alarm set and creation

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

int index=genRandInt(0,SO_PORTI-1);
kill(SIGALRM,ports[index].pidPort);
TEST_ERROR;
int elapsedDays=0; //elapsed days

while(elapsedDays<=SO_DAYS){
int index=genRandInt(0,SO_PORTI-1);
kill(SIGALRM,ports[index].pidPort);
TEST_ERROR;
elapsedDays++;

alarm(DAY_TIME);
}
//call dump

sleep(10);
deallocateResources();
}

//Functions definitions

/* 
Input: int max, int min
Output: int r random
Desc: return random int in range(max-min)
*/
int genRandInt(int max, int min){
srand(time(NULL));
int r = (rand() % (max - min + 1)) + min;

return r;
}

/* 
Input: void
Output: struct coords c
Desc: struct coords c with random x and y coords
*/
struct coords generateRandCoords(){

struct coords c;
//srand(time(0));
double div = RAND_MAX / SO_LATO;
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

struct port *ports = malloc(sizeof(struct port)*SO_PORTI);

int shmPort;
if((shmPort = shmget(PORT_POS_KEY,sizeof(ports),IPC_CREAT |IPC_EXCL|0666 )) == -1){
fprintf(stderr,"Error shared memory creation, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}

ports = (struct port *) shmat(shmPort,NULL,SHM_RDONLY);
if (ports == (void *) -1){
fprintf(stderr,"Error assing ports to shared memory, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
ports[0].coord.x = ports[0].coord.y = 0; //coord (0,0)
ports[1].coord.x = ports[1].coord.y = SO_LATO; //coord (SO_LATO,SO_LATO)
ports[2].coord.x = 0; //coord(0,SO_LATO)
ports[2].coord.y = SO_LATO;
ports[3].coord.x = SO_LATO; //coord(SO_LATO,0)
ports[3].coord.y = 0;
struct coords myC;
int i;
for (i=4;i<SO_PORTI;i++){ //generating SO_PORTI-4 ports
do{
myC=generateRandCoords(); 
}while(arrContains(ports,myC,i));
ports[i].coord=myC;
}

char x[50];
char y[50];

pid_t sonPid;
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
pid_t fork_ris;
int i;
pid_t ship_pid[1];

/*struct sembuf sops; inserire dentro ship
sops.sem_num=0;
sops.sem_op=-1;
sops.sem_flg=0;*/

for(i=0;i<SO_NAVI;i++){ 
switch(ship_pid[i]=fork()){
case -1:
fprintf(stderr,"Error in fork , %d: %s \n",errno,strerror(errno));
return -1;
case 0: 
//semop(sySem,&sops,1); da mettere dentro ship e porti
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
//SEMAPHORE 
int sySem;
if((sySem = semget(getpid(),1,IPC_CREAT|IPC_EXCL|0666)) == -1){
fprintf(stderr,"Error semaphore opened, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
if((sySem = shmctl(sySem,IPC_RMID,NULL)) == -1){
fprintf(stderr,"Error shared memory deallocation, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}

//SHARED MEMORY
int shmPort;
if((shmPort = shmget(PORT_POS_KEY,0,IPC_CREAT |IPC_EXCL|0666 )) == -1){
fprintf(stderr,"Error shared memory opened, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}
if((shmPort = shmctl(shmPort,IPC_RMID,NULL)) == -1){
fprintf(stderr,"Error shared memory deallocation, %d: %s\n",errno,strerror(errno));
exit(EXIT_FAILURE);
}

}

