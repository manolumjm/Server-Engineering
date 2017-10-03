#define _GNU_SOURCE
#define _CICLOS
#pragma GCC optimize("O0")
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <error.h>
#include <sched.h>
#include <netdb.h>
#include <time.h>

#define t_buf 10000000
char buf[t_buf];

int main()
{
    int port=7658;
    int datasocket;
    double serv, antser;
    int i;
    int *size = mmap(NULL,sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    int *barrier = mmap(NULL,sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    *buf=1;
    *size = 64;
    *barrier = 1;

    if(!fork())
    {
        struct sockaddr_in sock;

        datasocket = socket(AF_INET,SOCK_STREAM,0);
        sock.sin_family = AF_INET;
        sock.sin_addr.s_addr = inet_addr("127.0.0.1");
        sock.sin_port = htons(port);

        connect(datasocket,(struct sockaddr*)&sock,sizeof(sock));
        
        do{
            send(datasocket,buf,*size,0);
            while(*barrier);
            *barrier = 1;
        }while(*size);
        close(datasocket);
    }
    else
    {
        int cont_sock, from_len, so_reuseaddr = 1;
        struct sockaddr_in sock;

        cont_sock = socket(AF_INET,SOCK_STREAM,0);

        sock.sin_family = AF_INET;
        sock.sin_addr.s_addr = INADDR_ANY;
        sock.sin_port = htons(port);
        
        setsockopt(cont_sock,SOL_SOCKET,SO_REUSEADDR,&so_reuseaddr,sizeof(int));
        bind(cont_sock,(struct sockaddr*)&sock,sizeof(sock));
        listen(cont_sock,1);
        datasocket = accept(cont_sock,0,0);
        
        antser=0;
        while(*size)
        {
            serv=0;
            struct timespec t1, t2;
            clock_gettime(CLOCK_REALTIME,&t1);
            recv(datasocket,buf,*size,0);
            clock_gettime(CLOCK_REALTIME,&t2);
            serv += (double)((t2.tv_nsec-t1.tv_nsec)*1e-9+t2.tv_sec-t1.tv_sec);
            serv = *size / (serv*1024*1024);
            *size *= 2;
            if(serv<antser)
            {
                *size /= 4;
                serv = antser;
                printf("Velocidad máxima socket = %.3f MB/s\n", serv);
                printf("Paquete de tamaño %d bytes\n", *size);
                *size=0;
            }
            antser = serv;
            *barrier = 0;
        }
        close(datasocket);
        munmap(size, sizeof(int));
        munmap(barrier, sizeof(int));
    }

    return 0;
}
