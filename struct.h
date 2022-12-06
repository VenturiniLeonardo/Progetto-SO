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

typedef struct msgDemand{
    short type;
    int quantity;
}structMsgDemand;

typedef struct goods_states{
    int goods_in_port;
    int goods_on_ship;
    int goods_delivered;
    int goods_expired_port;
    int goods_expired_ship;
}struct_goods_states;

typedef struct goods_dump{
    struct goods_states [SO_MERCI];
}struct_goods_dump;