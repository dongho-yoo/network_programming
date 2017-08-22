/**
* Copyright (c) 2017 Wonderplanet Inc.
* Authorized by dongho.yoo.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "wp_sock.h"

int g_quit=0;

static void* peer_connected(void* param)
{
    int sockclient=(int)(intptr_t)param;

    char buffer[1024];
    while(1)
    {
       if (wps_recv(sockclient,buffer,sizeof(buffer),0)>0)
       {
           char send_buffer[64];

           printf("received from client\n");
           sprintf(send_buffer, "received %s", buffer);
           wps_send(sockclient, send_buffer, strlen(send_buffer)+1);
           wps_close(sockclient);
   
           /* クライアントからのメッセージが"quit"ならば、終了 */
           if (strcmp(buffer, "quit")==0)
           {
               g_quit=1;
               break;
           }
       }
    }
    return 0;
}

int main(int argc, char** argv)
{
    /* サーバらしくデーモンプロセスで。
       最終的にはforkと同様だが、ディレクトリ指定やstdoutの出力の設定をする場合、daemon関数は使いやすい */
    daemon(1,1);
    if (argv[1]==0)
    {
        printf("Usage : \n $> hellow_server [port number]\n");
        exit(0);
    }
    int port=atoi(argv[1]);
    sock_t sock=wps_listen(0,port,100,false);
    if (sock==-1)
    {
        /* エラー */
        fprintf(stderr,"listen error!!!\n");
        wps_close(sock);
        exit(0);
    }
    printf("listen success!!!\n");

    while(g_quit==0)
    {
        int ret=wps_wait(sock, 1000);
        if (ret==0)
        {
            /* タイムアウト */
            if (g_quit==1)
            {
                break;
            }
            continue;
        }
        printf("accept!!\n");
        sock_t sockclient=wps_accept(sock,0,0);
        if (sockclient==0||sockclient<0)
        {
            /* エラー */
            fprintf(stderr,"unexpected error!!!\n");
            exit(0);
        }
        pthread_t th;
        /* コネクションが確立した状態で、ここで処理を直接行うと次のコネクションまで時間がかかるので、
        ここはスレッドか子プロセスで分岐させるのが一般的 */
        if (pthread_create(&th,0,peer_connected,(void*)sockclient)!=0)
        {
            /* 各peerに関して、スレッド終了待ちをする必要がないので */
            pthread_detach(th);
        }
    }
    printf("terminate server!!!\n");

    return 0;
}

