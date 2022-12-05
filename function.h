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

//RANDOM COORDS GENERATOR FUNCTION
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

