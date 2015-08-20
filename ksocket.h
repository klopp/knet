/*
 * ksocket.h, part of "knet" project.
 *
 *  Created on: 20.05.2015, 02:41
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#ifndef KSOCKET_H_
#define KSOCKET_H_

#include "../klib/config.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#ifndef __WINDOWS__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#else
#include <winsock2.h>
#include <process.h>
#include "openssl/ssl.h"
#include "openssl/rand.h"
#if defined(_MSC_VER)
# include <io.h>
#endif
#endif

#define KSOCK_BUF_LEN           (1024*16)
#define KSOCK_DEFAULT_TIMEOUT   10

typedef struct _ksocket
{
    int timeout;
    int eof;
    int sock;
    int error;
    int ssl_error;
    int inbuf;
    int cursor;
    char buf[KSOCK_BUF_LEN];
    SSL_CTX *ctx;
    SSL *ssl;
}*ksocket;

int knet_init( void );
void knet_down( void );

ksocket knet_create_sd(ksocket sd, int timeout);
void knet_destroy_sd(ksocket sd);
int knet_connect( ksocket sd, const char * host, int port );
void knet_disconnect( ksocket sd );

int knet_read( ksocket sd, char *buf, size_t size );
int knet_write( ksocket sd, const void * buf, size_t size );
int knet_getc( ksocket sd );
int knet_init_ssl( ksocket sd, const SSL_METHOD * method );
const char * knet_error( ksocket sd );

#ifndef __WINDOWS__
# define    closesocket         close
# define    WSAGetLastError()   errno
# define    WSAEWOULDBLOCK      EINPROGRESS
# define	WSAETIMEDOUT		ETIME
# define    ioctlsocket         ioctl
# define    SOCKET_ERROR        -1
#endif

#endif /* KSOCKET_H_ */
