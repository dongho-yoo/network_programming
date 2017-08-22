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

   /* Hellow Worldを送る*/
   const char* kMessage = find_option_value("-m", argc, (const char**)argv);
   kMessage=kMessage?kMessage:"Hellow World";
   const size_t message_size = strlen(kMessage);

    /* 実際にはsendを使うべきだが、ファイルの読み書きと同じ形式であることを説明したかったので、writeで送信している。*/
    int write_bytes=write(sock,kMessage, message_size+1);
    if (write_bytes!=message_size+1)
    {
         /* 失敗*/
        printf("write error!!! (%d)", errno);
         exit(0);
    }

    /* 実際にはrecvを使うべきだが、ファイルの読み書きと同じ形式であることを説明したかったので、readで送信している。
       ストリームなので、実際にサーバから届いた分までを読み込む。read/recvの戻り値が読み込んだバイト数になる。
       ある意味、ストリームなので、どれくらい読み込めばいいのか分からないので、送信ヘッダーにサイズを入れるのが一般的であるが、
       このサンプルは流れを説明するのが目的なので省いている。
       チャットのクライアントとサーバを作る際にこの件に関して細かい説明を行う予定。
       read/recvは基本的にブロック型なので、返事がくるまで待ち状態になる。 
    */
    char buffer[1024];
    int n=read(sock, buffer, 1024);
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
    /* ディスクリプターを解放します。 */
    close(sock);

    return 0;
}

