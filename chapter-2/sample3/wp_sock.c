/**
* Copyright (c) 2017 Wonderplanet Inc. All Rights Reserved.
* Authorized by dongho.yoo.
*/
#include "wp_sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>

#define PRINT_ERROR(...) fprintf(stdout,__VA_ARGS__)

static sock_t tcp_sock()
{
    sock_t sock;
#ifndef MSG_NOSIGNAL
    /* BSD系はこれを指定しないと、write時にサーバ側からデータがくるとシグナルが発生して、
    プロセスが終了する。*/
    signal(SIGPIPE, SIG_IGN);
#endif

    if ((sock= socket(AF_INET, SOCK_STREAM, 0))<0)    
    {
        PRINT_ERROR("socket create error !");
        return -1;
    }
    return sock;
}
/** ディスクリプターをノンブロックする関数*/
static void sock_non_block(sock_t sock, int nonblock)
{
    fcntl(sock, F_SETFL, nonblock?O_NONBLOCK:0);
}
static int is_valid_sock(int sock)
{
    int error=0;
    socklen_t len;

    if (sock==0)
    {
        return 0;
    }
    /* ソケットが無効化どうかをチェックする */
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len)!=0)
    {
        return errno;
    }

    if (error==0 ||
            error==EISCONN)
    {
        return 1;
    }

    PRINT_ERROR("socket invalid reason:%d", error);
    return 0;
}
static int is_ready_connect(int sock, int timeout)
{
    int res=0;
    fd_set fds;
    struct timeval tv={timeout/1000,(timeout%1000)*1000};

    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    /* selectでディスクリプターが書き込み可能になるまで待つ。
       MAN CONNECT(2)を参照した。
    */
    res = select(sock+1, 0, &fds, 0, &tv);

    if (res<0)
    {
        PRINT_ERROR("select error errno:%d", errno);
        return res;
    }

    if (res==0)
    {
        return 0;
    }
    if (FD_ISSET(sock, &fds)!=0)
    {
        if (is_valid_sock(sock))
        {
            return 1;
        }
    }

    return 0;
}
/** connectのタイムアウト版 
    複雑に見えるが、実際にはTCPのHandshake終了待ちを手動で行っているだけ。
*/
sock_t wps_connect_s(const char* ipaddress, int port, int timeout)
{
    return wps_connect(inet_addr(ipaddress),port,timeout);
}
sock_t wps_connect(addr_ipv4_t ipaddress, int port, int timeout)
{
    int sock;
    struct sockaddr_in addr={0,};
    int val;
    int retryCnt=timeout/1000;

#ifndef MSG_NOSIGNAL
    /* BSD系はこれを指定しないと、write時にサーバ側からデータがくるとシグナルが発生して、
    プロセスが終了する。*/
    signal(SIGPIPE, SIG_IGN);
#endif

    if ((sock=tcp_sock())<0)    
    {
        PRINT_ERROR("socket create error !");
        return -1;
    }

    if (timeout!=0)
    {
        val = 1;
        /*  タイムアウトにする場合には、一旦ソケットをノンブロック状態にする。*/
        sock_non_block(sock, 1);
    }

    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=ipaddress;

__RETRY:
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))!=0)
    {
        if (timeout==0)
        {
            PRINT_ERROR("connect error!\n");
            return -1;
        }

        if (errno==EAGAIN)
        {
            /* ノンブロック時に発生するエラーでこれは仕様なのでリトライ*/
            goto __RETRY;
        }
        if (errno==EINPROGRESS)
        {
            /* まだ書き込み可能な状態でない場合のエラー */
            int res;
__WAIT:
            /* この中で書き込み可能になるまで待つ。*/
            res = is_ready_connect(sock, 1000); 
            if (res<0)
            {
                PRINT_ERROR("connect error... (%d)", errno);
                return -1;
            }
            if (res==0)
            {
                if (retryCnt--!=0)
                {
                    PRINT_ERROR("timeout so... retry");
                    goto __WAIT;
                }
                // retryCnt is zero.
                PRINT_ERROR("connect timeout (%d)", errno);
                return 0;
            }

            /* ノンブロック解除 */
            sock_non_block(sock, 0);
            return sock;
        } /* if EINPROGRESS */
        return 0;
    } /* connect retry or failure. */
    
    return sock;
}
static int __send(sock_t sock, const void* data, const size_t data_size)
{
#ifdef MSG_NOSIGNAL
    return send(sock,data, data_size,MSG_NOSIGNAL);
#else
    return send(sock,data, data_size,0);
#endif
}
int wps_send(sock_t sock,const void* data,const size_t data_size)
{
    size_t sum=data_size;
    char* p=(char*)data;
    /* WRITE_ONCE_BYTES ずつ書き込む */
    while(sum!=0)
    {
        const size_t size=(sum<WRITE_ONCE_BYTES)?sum:WRITE_ONCE_BYTES;
        int n =__send(sock, p, size);
        p+=n;
        sum-=n;
    }
    return data_size;
}
int wps_recv(sock_t sock, void* buffer, const size_t buffer_size, int timeout)
{
    int sum=buffer_size;
    char* p=(char*)buffer;
    if (timeout!=0)
    {
        fd_set fdz;
        struct timeval tv={timeout/1000,(timeout%1000)*1000};
        FD_ZERO(&fdz);
        FD_SET(sock,&fdz);
        int res = select(sock+1,&fdz,0,0,&tv);
        if (res<0)
        {
            /* エラー */
            return -2;
        }
        if (res==0)
        {
            /* タイムアウト */
            return -1;
        }
    }
    /* recvに関しては今の状態でREAD_ONCE_BYTESずつ読み込むのは不可能。
    その理由は、相手が送ったデータのサイズがわからないためである。
    なので、recvの方は、プロトコルの実装のところで、READ_ONCE_BYTESずつ読み込む処理を入れる。*/
    return recv(sock,buffer,buffer_size,0);
}
sock_t wps_listen(const char* ipaddress, int port, int backlog, int force)
{
    sock_t sock;
    struct sockaddr_in addr={0,};
    const size_t addr_size = sizeof(addr);

    if ((sock=tcp_sock())<0)    
    {
        PRINT_ERROR("socket create error !");
        return -1;
    }

    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=ipaddress?inet_addr(ipaddress):htonl(INADDR_ANY);

    if (bind(sock,(struct sockaddr*)&addr, addr_size)==-1)
    {
        if ((errno==EADDRINUSE||errno==EACCES)&&force==1)
        {
            /* ポートが解放されなかったり、すでに使われている場合に、force==1の場合は強制的にバインドする */
		    int on=1;
		    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
            if (bind(sock, (struct sockaddr*)&addr, addr_size)==-1)
            {
                /* これでもダメならお手上げ！*/
                PRINT_ERROR("bind error!!!(%d)\n",errno);
                return -1;
            }
        }
        else
        {
            PRINT_ERROR("bind error!!!(%d)\n",errno);
            close(sock);
            return -1;
        }
    }    
    if (listen(sock, backlog)==-1)
    {
        /* エラー */
        PRINT_ERROR("listen error!!!(%d)\n",errno);
        close(sock);
        return -1;
    }

    return sock;
}
sock_t wps_accept(sock_t sock_for_listen,addr_ipv4_t* ip_address, int* port)
{
    struct sockaddr_in accept_addr={0,};
    socklen_t accpet_addr_size=sizeof(accept_addr);
    int sock_client=accept(sock_for_listen,(struct sockaddr*)&accept_addr, &accpet_addr_size);
    if (ip_address!=0)
    {
        *ip_address=accept_addr.sin_addr.s_addr;
    }
    if (port!=0)
    {
        *port=ntohs(accept_addr.sin_port);;
    }

    return sock_client;
}
void wps_close(sock_t sock)
{
    close(sock);
}
int wps_wait(sock_t sock,int timeout)
{
    int res=0;
    fd_set fdz;
    struct timeval tv={timeout/1000,(timeout%1000)*1000};

    FD_ZERO(&fdz);
    FD_SET(sock, &fdz);

    /* 汎用的に待つ処理をするので、読み込み時と書き込み時、両方待つようにする */
    return select(sock+1, &fdz, &fdz, 0, &tv);
}
