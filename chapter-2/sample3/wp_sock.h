/**
* Copyright (c) 2017 Wonderplanet Inc. All Rights Reserved.
* Authorized by dongho.yoo.
*/

#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

/** @brief ソケットのディスクリプターの型を定義 */
typedef int sock_t;
typedef int addr_ipv4_t;

/* 一度に読み込むバッファーのサイズ。システムによって最適な値がある。これは実証値を使うべきだが、一旦4KBytesで。 */
#define WRITE_ONCE_BYTES (4096)

/* 一度に読み込むバッファーのサイズ */
#define READ_ONCE_BYTES (4096)

/**
* @brief サーバへ接続します。
* @param ipaddress 文字列のIPアドレス
* @param port ポート番号
* @param timeout タイムアウト（ミリ秒）、timeoutがゼロの場合はブロック
* @return ソケットのディスクリプター
*/
extern sock_t wps_connect_s(const char* ipaddress, int port, int timeout);
/**
* @brief サーバへ接続します。
* @param ipaddress ビッグエンディアンの32ビットのipv4のipアドレス
* @param port ポート番号
* @param timeout タイムアウト（ミリ秒）、timeoutがゼロの場合はブロック
* @return ソケットのディスクリプター
* @retval -1 ソケット生成失敗
* @retval -2 タイムアウト
*/
extern sock_t wps_connect(addr_ipv4_t ipaddress, int port, int timeout);
/**
* @brief データを送信します。
* @param data 送信するデータ
* @param data_size 送信するデータサイズ
* @return 送信したデータのバイト数
*/
extern int wps_send(sock_t sock, const void* data, const size_t data_size);
/**
* @brief データを受信します。
* @param data 受信するバッファー
* @param data_size 受信するバッファーサイズ
* @param timeout ミリ秒、ゼロの場合はブロック
*/
extern int wps_recv(sock_t sock, void* buffer, const size_t buffer_size, int timeout);
/**
* @brief リッスン状態のソケットを生成します。
* @param ipaddress IPアドレス、NULLの場合はデフォルトのIPアドレスで取得。
* @param port 待ち受けるポート番号
* @param backlog TCPハンドジェーク終了時のクライアントをacceptまで貯められる数
* @param force 1の場合は、ポートがすでに使われていたり、解放されてない場合にも強制的にバインドする
* @return ソケットディスクリプター
*/
extern sock_t wps_listen(const char* ipaddress, int port, int backlog, int force);
/**
* @brief 接続を確立させて、クライアントとのディスクリプターを生成する
* @param sock_for_listen リッスン状態のディスクリプター
* @param ip_address 受け取るクライアントのipアドレス（NULLの場合は無視される）
* @param port 受け取るクライアントのport番号（NULLの場合は無視される）
*/
extern sock_t wps_accept(sock_t sock_for_listen, addr_ipv4_t* ip_address, int* port);
/**
* @brief ディスクリプターを削除します。
* @param sock ディスクリプター
*/
extern void wps_close(sock_t sock);
/**
* @brief ソケットの受信を待ちます。
* @param sock ディスクリプター
* @param timeout タイムアウト
* @return 0ならタイムアウト
*/
extern int wps_wait(sock_t sock,int timeout);
#ifdef __cplusplus
}
#endif

