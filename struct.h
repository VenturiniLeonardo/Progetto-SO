struct{
    unsigned short sem_num;
    short sem_op;
    short sem_flg;
}sembuf;

struct coords{
    double x;
    double y;
};

struct port{
    pid_t pidPort;
    struct coords coord;
};


