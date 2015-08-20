/*
 * ksmtp.h, part of "knet" project.
 *
 *  Created on: 18.08.2015, 20:27
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#ifndef KSMTP_H_
#define KSMTP_H_

#include "ksocket.h"
#include "../stringlib/stringlib.h"
#include <limits.h>

typedef struct _KSmtp
{
    int verbose;
    struct _ksocket sd;
#ifndef __WINDOWS__
    char node[HOST_NAME_MAX + 1];
#else
    char node[ PATH_MAX + 1 ];
#endif
    string error;
    string current;
}*KSmtp;

KSmtp smtp_Create( int timeout, const char * node, int verbose );

int smtp_OpenSession( KSmtp smtp, const char * host, int port, int tls );
void smtp_CloseSession( KSmtp smtp );

int smtp_AUTH_LOGIN( KSmtp smtp, const char * user, const char * password );
int smtp_AUTH_PLAIN( KSmtp smtp, const char * user, const char * password );
int smtp_MAIL_FROM( KSmtp smtp, const char * email );
int smtp_RCPT_TO( KSmtp smtp, const char *email );
int smtp_write_buf( KSmtp smtp, const void * data, size_t size );
int smtp_write( KSmtp smtp, const char * data );
int smtp_DATA( KSmtp smtp );
int smtp_END_DATA( KSmtp smtp );

void smtp_Destroy( KSmtp smtp );

#endif /* _KSMTP_H_ */
