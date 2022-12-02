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
struct coords generateRandCoords();

int main(){
    struct coords ship_coords;
    ship_coords=generateRandCoords();

}


//Functions definitions

/*
Input: void
Output: 
Desc: c
*/
struct coords generateRandCoords(){
    struct coords c;
    srand(time(0));
    double dec=rand()/RAND_MAX;
    c.x = dec*SO_LATO;
    dec=rand()/RAND_MAX;
    c.y = dec*SO_LATO;
    return c;
    
}

