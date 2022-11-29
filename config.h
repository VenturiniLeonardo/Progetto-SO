//Configuration file with all parameters

//GENERALS
#define SO_DAYS     //number of simulation days

//PORTS
#define SO_PORTI    //number of ports (int,>= 4)
#define SO_BANCHINE     //maximum number of docks (int)
#define SO_FILL     //maximum cargo capacity of the port
#define SO_LOADSPEED    //loading/unloading speed of ports (ton/days)

//SHIPS
#define SO_NAVI     //number of ships (int,>=1)
#define SO_SPEED    //speed of ships (double or int)
#define SO_CAPACITY     //capacity of ships (ton)

//CARGO
#define SO_MERCI    //type of cargo (int)
#define SO_SIZE     //weight of cargo (ton)
#define SO_MIN_VITA     //minimum expiry date (days)
#define SO_MAX_VITA     //maximum expiry date (days)

//MAP
#define SO_LATO     //side of the map (double)

//WEATHER EVENTS
#define SO_STORM_DURATION   //duration of storm (hours)
#define SO_SWELL_DURATION   //duration of swell (hours)
#define SO_MEALSTROME   //mealtrome repeat (hours)