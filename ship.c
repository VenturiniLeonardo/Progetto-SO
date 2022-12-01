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

int main(){
    struct coords ship_coords;
    srand(time(0));
    double div=rand()/SO_LATO;
    ship_coords.x=div-(int)div;
    div=rand()/SO_LATO;
    ship_coords.y=div-(int)div;
    
}


