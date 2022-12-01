//Configuration file with all parameters

//GENERALS
#define SO_DAYS    //number of simulation days

//PORTS
#define SO_PORTI 9    //number of ports (int,>= 4)
#define SO_BANCHINE     //maximum number of docks (int)
#define SO_FILL         //maximum goods capacity of the port
#define SO_LOADSPEED    //loading/unloading speed of ports (ton/days)

//SHIPS
#define SO_NAVI  1      //number of ships (int,>=1)
#define SO_SPEED        //speed of ships (double or int)
#define SO_CAPACITY     //capacity of ships (ton)

//GOODS
#define SO_MERCI        //type of goods (int)
#define SO_SIZE         //weight of goods (ton)
#define SO_MIN_VITA     //minimum expiry date (days)
#define SO_MAX_VITA     //maximum expiry date (days)

//MAP
#define SO_LATO 0.000001   //side of the map (double)

//WEATHER EVENTS
#define SO_STORM_DURATION   //duration of storm (hours)
#define SO_SWELL_DURATION   //duration of swell (hours)
#define SO_MEALSTROM        //mealstrom repeat (hours)

//CHECK FUNCTION
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

//TEST ERROR FUNCTION
#define TEST_ERROR  if(errno) {fprintf(stderr, \
                            "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}
