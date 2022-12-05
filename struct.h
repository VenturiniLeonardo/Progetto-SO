typedef struct{
    unsigned short sem_num;
    short sem_op;
    short sem_flg;
} sembuf;

typedef struct coords{
    double x;
    double y;
}structCoords;

typedef struct port{
    pid_t pidPort;
    struct coords coord;
}structPort;

typedef struct good{
    int id;
    int type;
    int quantity;
    int date_expiry;
    struct good* next;
}structGood;

typedef struct supplyDemandPending{
    struct good *supply;
    struct good *demand;
    struct good *pending;
    
} structSupplyDemand;
