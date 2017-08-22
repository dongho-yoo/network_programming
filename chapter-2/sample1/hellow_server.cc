/**
* Copyright (c) 2017 Wonderplanet Inc.
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

int main(int argc, char** argv)
{
    /* 
    サーバの構造は以下のとおりである。
    1. ディスクリプターを生成する
    2. ポート番号を結びつける処理
    3. クライアントからの接続を待ち受け、接続を確立する処理
    4. クライアントの接続からディスクリプターを生成する処理
    5. 送信・受信処理、3に戻り接続待ち
    */
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock<0)
    {
        /* TODO ソケットが生成されない場合のエラー処理 */
        printf("socket failure!!! (%d)", errno);
        exit(0);
    }
    if (argv[1]==0)
    {
        printf("Usage : \n $> hellow_server [port number]\n");
        exit(0);
    }
    int port=atoi(argv[1]);
    /* ここからがバインド処理 */
    struct sockaddr_in addr={0,};
    addr.sin_family = AF_INET;
    /* 待ち受けるポート番号。ビッグエンディアンの32ビットに変換*/
    addr.sin_port = htons(port);
    /* アドレスが複数ある場合は指定するが、そうでもない場合は以下のように適当にアドレスを指定する */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    const size_t addr_size = sizeof(addr);

    /* 引数はクライアントのconnectに似ている */
    if (bind(sock, (struct sockaddr*)&addr, addr_size)==-1)
    {
        /* ポートが解放されず、バインドされないケースもある。テスト時はプロセスが落ちたりするので、ポートが解放されない場合がある。
           そういう場合は、sockoptでSO_REUSEADDRを使うべき */
        printf("bind error!!!(%d)\n",errno);
        close(sock);
        exit(0);
    }    
    /* クライアントの待ち受け
     listenの第二引数はbacklogである。筆者がソケットプログラミングを始めた頃では接続がまだ確立されていない状態のクライアントの接続のキューの数だったが、最近は接続確立済みのキューの数になっているようだ。
     なので、以下の10では、クライアントのコネクション待ち状態は最大10コネクションになり、11番目のコネクションからはコネクトエラーになる。*/
    if (listen(sock, 10)==-1)
    {
        /* エラー */
        printf("listen error!!!(%d)\n",errno);
        close(sock);
        exit(0);
    }
    printf("listen success!!!\n");
ACCEPT:
    /* ディスクリプターsockは、クライアントの待ち受け状態になっている。listenがブロックして待っているわけではない。
       acceptでブロックして待つことになる。
       connectとbindと引数の指定の仕方が少し違うのに注目。
       acceptが成功した場合、accept_addrにはクライアントのipアドレスやポート番号などの値が入ってくる。
       */
    struct sockaddr_in accept_addr={0,};
    socklen_t accpet_addr_size=sizeof(accept_addr);
    int sock_client=accept(sock,(struct sockaddr*)&accept_addr, &accpet_addr_size);

    if (sock_client==0||sock_client<0)
    {
        /* エラー */
        exit(0);
    }

    char buffer[1024];
    if (recv(sock_client,buffer,sizeof(buffer),0)>0)
    {
        char send_buffer[64];
        sprintf(send_buffer, "received %s", buffer);
        send(sock_client, send_buffer, strlen(send_buffer)+1, 0);
        close(sock_client);

        /* クライアントからのメッセージが"quit"ならば、終了 */
        if (strcmp(buffer, "quit")==0)
        {
            close(sock);
            return 0;
        }
    }
    goto ACCEPT;
    return 0;
}

