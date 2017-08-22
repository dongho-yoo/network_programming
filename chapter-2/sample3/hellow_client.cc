/**
* Copyright (c) 2017 Wonderplanet Inc. All Rights Reserved.
* Authorized by dongho.yoo.
*/
#include "wp_sock.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* find_option_value(const char* option, int argc, const char** argv)
{
    for (int i=1;i<argc;++i)
    {
        if (strcmp(argv[i],"-m")==0)
        {
            return argv[i+1];
        }
    }
    return 0;
}

/* クライアントの流れは以下の通りである
1. ディスクリプターを生成する
2. ポート番号を結びつける処理 (bindだがポート番号が任意でいいのならば、connectで任意のポートを決めるので省くのが一般的)
3. サーバへ接続
4. サーバとのやりとり
*/
int main(int argc, char** argv)
{
    sock_t sock;
    if (argv[1]==0&&argv[2]==0)
    {
        printf("Usage : \n $> hellow_client [server ipaddress] [port number]\n");
        exit(0);
    }
    unsigned short port=(short)atoi(argv[2]);
    const char* ip_address=argv[1];

    if ((sock=wps_connect_s(ip_address, (int)port, 5000))<0)
    {
        fprintf(stderr,"connect error!!!\n");
        exit(0);
    }

    const char* kMessage = find_option_value("-m", argc, (const char**)argv);
    kMessage=kMessage?kMessage:"Hellow World";
    const size_t message_size = strlen(kMessage);

    int write_bytes=wps_send(sock,kMessage, message_size+1);
    if (write_bytes!=message_size+1)
    {
        /* 失敗*/
        printf("send error!!!\n");
        exit(0);
    }
    printf("send success\n");

    char buffer[1024];
    int n=wps_recv(sock, buffer, 1024,3000);
    if (n<0)
    {
        /* エラー */
        printf("recv error!!!\n");
        exit(0);
    }
    if (n==0)
    {
        /* 接続が切れた! */
        printf("connection closed !!!\n");
        exit(0);
    }
 
    printf("%s\n", buffer);
    /* ディスクリプターを解放します。 */
    wps_close(sock);

    return 0;
}

