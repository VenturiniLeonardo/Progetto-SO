/*Configuration file with all parameters*/

/*GENERALS*/
#define SO_DAYS 10 /*number of simulation days*/
#define DAY_TIME 1 /*day duration in seconds*/

/*PORTS*/
#define SO_PORTI 4 /*number of ports (int,>= 4)*/
#define SO_BANCHINE 5 /*maximum number of docks (int)*/
#define SO_FILL 100 /*maximum goods capacity of the port*/
#define SO_LOADSPEED 3/*loading/unloading speed of ports (ton/days)*/
#define SO_DISTANZA 0.0001 /*minimum distance between two ports*/

/*SHIPS*/
#define SO_NAVI 2 /*number of ships (int,>=1)*/
#define SO_SPEED 10 /*speed of ships (double or int)*/
#define SO_CAPACITY 15/*capacity of ships (ton)*/

/*GOODS*/
#define SO_MERCI 2 /*type of goods (int)*/
#define SO_SIZE 100 /*weight of goods (ton)*/
#define SO_MIN_VITA 2 /*minimum expiry date (days)*/
#define SO_MAX_VITA 10 /*maximum expiry date (days)*/

/*MAP*/
#define SO_LATO 15 /*side of the map (double)*/

/*WEATHER EVENTS*/
#define SO_STORM_DURATION /*duration of storm (hours)*/
#define SO_SWELL_DURATION 2 /*duration of swell (hours)*/
#define SO_MEALSTROM /*mealstrom repeat (hours)*/

/*KEY SHARE MEMORY*/
#define PORT_POS_KEY 58 /*key for shared memory contains array coords*/
#define GOODS_DUMP_KEY 30
#define PORT_DUMP_KEY 25
#define SHIP_DUMP_KEY 35

/*KEY MESSAGGE QUEUES*/
#define PORT_MASTER_KEY 23 /*key for shared memory contains array coords*/

/*KEY SEMAPHORE*/
#define DUMP_KEY 42
#define SY_KEY 1

/*CHECK FUNCTION*/
/*
Input: void
Output: void
Desc: check define value of file config.h
*/

#define PARAM_CHECK 
void checkParams(){
if(SO_PORTI<4){
printf("Input error: SO_PORTI < 4\n");
exit(EXIT_FAILURE);
} 
if(SO_NAVI<1){
printf("Input error: SO_NAVI < 1\n");
exit(EXIT_FAILURE);
} 
if(SO_LATO<=0){
printf("Input error: SO_LATO cannot be negative or zero\n");
exit(EXIT_FAILURE);
} 

/*TODO: check di quante permutazioni ci sono per verificare che ci stiano SO_PORTI nella mappa*/
}

/*TEST ERROR FUNCTION*/
#define TEST_ERROR if(errno) {fprintf(stderr, \
"%s:%d: PID=%5d: Error %d (%s)\n",\
__FILE__,\
__LINE__,\
getpid(),\
errno,\
strerror(errno));}



