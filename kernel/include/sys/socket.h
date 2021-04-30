#ifndef __SYS_SOCKET_H__
#define __SYS_SOCKET_H__

#define INADDR_ANY  0

enum {
    AF_INET = 2,
};

enum {
    SOCK_STREAM = 1,
    SOCK_DGRAM  = 2
};

enum {
    MSG_DONTWAIT = 1 << 0
};

typedef unsigned int socklen_t;

typedef struct in_addr {
    unsigned long s_addr;
} in_addr_t;

typedef struct sockaddr_in {
    short int          sin_family;
    unsigned short int sin_port;
    struct in_addr     sin_addr;
    unsigned char      sin_zero[8];
} saddr_in_t;

typedef struct sockaddr {
    short int     sa_family;
    unsigned char sa_data[14];
} saddr_t;

#endif /* __SYS_SOCKET_H__ */
