/**
* Copyright (c) 2017 Wonderplanet Inc. All Rights Reserved.
* Authorized by dongho.yoo.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>

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
   /* ソケットを生成します。
       第一引数のAF_INETはipv4のTCPプロトコルです。
       他にも色々あるが、話が長くなるので、今のところではAF_INETとAF_UNIXのみ覚えていただきたい。
       AF_UNIXはドメインソケットであり、内部ではnamed pipeになっているので高速である 
       第二引数はのSOCK_STREAMはTCPを意味する。言葉通りストリーム形式で入ってくる。
       UDPの場合はSOCK_DGRAMを指定し、イーサネットフレームレベルの制御がしたいならば、SOCK_RAWを指定する*/
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock<0)
    {
        /* TODO ソケットが生成されない場合のエラー処理 */
        printf("socket failure!!! (%d)", errno);
        exit(0);
    }
    if (argv[1]==0&&argv[2]==0)
    {
        printf("Usage : \n $> hellow_client [server ipaddress] [port number]\n");
        exit(0);
    }
    /* ディスクリプターが生成されたので、サーバに接続する */ 
    unsigned short port=(short)atoi(argv[2]);
    const char* ip_address=argv[1];
    struct sockaddr_in addr={0,};

    /* 接続のために構造体に値を入れる */
    addr.sin_family = AF_INET;
    /* 接続先のポート番号。ビッグエンディアンの32ビットに変換*/
    addr.sin_port = htons(port);
    /* inet_addrはxxx.xxx.xxx.xxxの文字列をビッグエンディアン(ネットワークバイトオーダー)の32ビットに変換する。*/
    addr.sin_addr.s_addr = inet_addr(ip_address); 

    /* サーバにコネクトを行う。この関数はブロック型である。
    　 コネクトのタイムアウトはデバイスドライバーの実装によって違う。
       このタイムアウト値の設定もくせものでOSによっては設定ができない。
       なので、タイムアウトをさせる場合は、別途実装する必要があるが、色々と説明が長くなるので
       今回の連載では説明は省いて、どのような実装になるのか最終的に見せたいと思っている。 

       第二引数のstruct sockaddrのキャストだが、sockの第一引数に何を指定しているかで指定される構造体が違うためにキャストすることになる。
       現代のソフトウェア工学的には気持ち悪い実装かも知れないが、connectがシステムコールである故にやむを得ないところがある。
    */
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr))!=0)
    {
        /* 失敗*/
        printf("socket connect error!!! (%d)", errno);
        exit(0);
    }

/* Free BSD系やLinux 2.2以降でない場合は、送信時に送信先が何かを送信したり、接続が切れた場合、SIGPIPEが発生しする。
その対策 */
#ifndef MSG_NOSIGNAL
    /* SIGPIPEを無視する */
    signal(SIGPIPE, SIG_IGN);
#endif

   /* Hellow Worldを送る*/
   const char* kMessage = find_option_value("-m", argc, (const char**)argv);
   kMessage=kMessage?kMessage:"Hellow World";
   const size_t message_size = strlen(kMessage);

    /* BSD系とLinux系ではMSG_NOSIGNALを設定する */
#ifdef MSG_NOSIGNAL
    /* 第三引数に注目してほしい。第三引数には、MSG_NOSIGNAL、MSG_OOB、MSG_DONTROUTEがある。
    　 いづれもなければゼロで問題はない。
    　 MSG_OOBはOut Of Bandであるが、説明が長くなるので、今回は省く。
       MSG_DONTROUTEは、ルーター向けと思ってほしい。これも今回は省く。
    */
    int write_bytes=send(sock,kMessage, message_size+1,MSG_NOSIGNAL);
#else
    int write_bytes=send(sock,kMessage, message_size+1,0);
#endif
    if (write_bytes!=message_size+1)
    {
         /* 失敗*/
        printf("write error!!! (%d)", errno);
         exit(0);
    }

    /* タイムアウト処理 */
    fd_set fdz;
    struct timeval tv = {3,0}; /* タイムアウト3秒 */

    /* このFD_で始まるマクロに注目。
       実は、OSによってselectの実装が異なるために、fd_setという構造体の型もバラバラである。
       そのためにFD_で始まるマクロを利用し、使い方を統一している。
       以下のマクロはfdzにディスクリプターを設定する。
       実際には複数のディスクリプターを設定できて、どのディスクリプターからの受信したのかが分かるようになる。
       この際に設定できるディスクリプターのタイプだが、ソケット以外にもselect対応のデバイスは全て一緒に設定できる。
       その使い方は次回のネットワークフレームワーク作る際に紹介する予定*/
    FD_ZERO(&fdz); /* fdzの初期化 */
    FD_SET(sock,&fdz); /* fdzにディスクリプターを設定 */

    /*
    第一引数は、FD_SETで設定されたディスクリプターの最も大きい値+1が入る。
    第二引数は受信待ちのディスクリプター、第三引数は書き込み待ち時のディスクリプター、第四引数はエラー検出を行うディスクリプターである。
    第五引数はタイムアウト値である。
    selectは、内部的にfdz内のディスクリプターやカーネルのタイマーを設定してからプロセス（スレッド）をスリープさせる。
    するとデバイスのタイムスライスの中で、このディスクリプターに書き込みがあった際に、結びついているプロセス（スレッド）を起こす。
    または、カーネルのタイマーがプロセス（スレッド）を起こすことになる。
    その際に、selectの中では、fdzに設定されているディスクリプターで書き込みがあるディスクリプターの数を数えて、その数をリターンする。
    もし、そういうディスクリプターがない場合は、タイムアウトによるものであるので、ゼロが返ってくる。
    */
    int n=select(sock+1, &fdz, 0, 0, &tv);

    if (n==0)
    {
        /* タイムアウト */
        fprintf(stderr,"Timout!!!!\n");
        close(sock);
        exit(0);
    }
    if (n<0)
    {
        /* エラー */
        close(sock);
        exit(0);
    }
    if (n==1)
    {
        /* こんな感じで、どのディスクリプターに受信されたかをチェックできる。
           このプログラムでは、sockしかないので無意味ではあるが。。。 */
        if (FD_ISSET(sock,&fdz))
        {
            char buffer[1024];
            int n=recv(sock, buffer, 1024,0);
            if (n<0)
            {
                 /* エラー */
                printf("read error!!! (%d)", errno);
                 exit(0);
            }
            if (n==0)
            {
                 /* 接続が切れた! */
                printf("connect closed!!! (%d)", errno);
                 exit(0);
            }
            printf("%s\n", buffer);
         }
    }

    /* ディスクリプターを解放します。 */
    close(sock);

    return 0;
}

