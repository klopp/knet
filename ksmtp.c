/*
 * ksmtp.c, part of "knet" project.
 *
 *  Created on: 18.08.2015, 21:58
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#include "ksmtp.h"
#include "ksmtp.h"

#include "../stringlib/b64.h"

static int smtp_answer( KSmtp smtp )
{
    char buf[8];
    do
    {
        int c;
        memset( buf, 0, sizeof(buf) );
        if( knet_read( &smtp->sd, buf, 4 ) != 4 )
        {
            sprint( smtp->error, "smtp_answer(1): %s",
                    knet_error( &smtp->sd ) );
            return 0;
        }
        scpyc( smtp->current, buf );
        while( (c = knet_getc( &smtp->sd )) != '\n' && !smtp->sd.eof/*!knet_eof( &smtp->sd )*/)
        {
            if( c == -1 )
            {
                sprint( smtp->error, "smtp_answer(2): %s",
                        knet_error( &smtp->sd ) );
                return 0;
            }
            else
            {
                scatch( smtp->current, c );
            }
        }
        if( smtp->verbose )
        {
            schomp( smtp->current );
            fprintf( stderr, "<<< %s\n", sstr( smtp->current ) );
        }

    } while( buf[3] != ' ' );

    return (buf[0] - '0') * 100 + (buf[1] - '0') * 10 + (buf[2] - '0');
}

int smtp_write( KSmtp smtp, const char * buf )
{
    if( smtp->verbose )
    {
        scpyc( smtp->current, buf );
        schomp( smtp->current );
        fprintf( stderr, ">>> %s\n", sstr( smtp->current ) );
    }
    knet_write( &smtp->sd, buf, strlen( buf ) );
    if( smtp->sd.error/*knet_error( &smtp->sd )*/)
    {
        sprint( smtp->error, "smtp_write(): %s", knet_error( &smtp->sd ) );
        return 0;
    }
    return 1;
}

static int smtp_cmd( KSmtp smtp, const char * cmd, int ok, int ko )
{
    int rc;
    if( cmd && !smtp_write( smtp, cmd ) ) return 0;
    rc = smtp_answer( smtp );
    if( !rc ) return 0;
    if( rc == ok ) return rc;
    if( ko && rc == ko ) return rc;
    schomp( smtp->current );
    if( ko )
    {
        sprint( smtp->error, "smtp_cmd(): want %u or %u, got [%s]", ok, ko,
                sstr( smtp->current ) );
    }
    else
    {
        sprint( smtp->error, "smtp_cmd(): want %u, got [%s]", ok,
                sstr( smtp->current ) );
    }
    return 0;
}

static int smtp_helo( KSmtp smtp )
{
    int rc;
    string buf = snew();
    sprint( buf, "HELO %s\r\n", smtp->node );
    rc = smtp_cmd( smtp, sstr( buf ), 250, 0 );
    sdel( buf );
    return rc;
}

static int smtp_ehlo( KSmtp smtp )
{
    int rc;
    string buf = snew();
    sprint( buf, "EHLO %s\r\n", smtp->node );
    rc = smtp_cmd( smtp, sstr( buf ), 250, 0 );
    sdel( buf );
    return rc;
}

int smtp_DATA( KSmtp smtp )
{
    return smtp_cmd( smtp, "DATA\r\n", 354, 0 );
}

int smtp_END_DATA( KSmtp smtp )
{
    return smtp_cmd( smtp, "\r\n.\r\n", 250, 0 );
}

int smtp_MAIL_FROM( KSmtp smtp, const char * email )
{
    int rc;
    string buf = snew();
    sprint( buf, "MAIL FROM:<%s>\r\n", email );
    rc = smtp_cmd( smtp, sstr( buf ), 250, 0 );
    sdel( buf );
    return rc;
}

int smtp_RCPT_TO( KSmtp smtp, const char *email )
{
    int rc;
    string buf = snew();
    sprint( buf, "RCPT TO:<%s>\r\n", email );
    rc = smtp_cmd( smtp, sstr( buf ), 250, 251 );
    sdel( buf );
    return rc;
}

int smtp_AUTH_LOGIN( KSmtp smtp, const char * user, const char * password )
{
    int rc = 0;
    string buf = snew();
    string data = base64_sencode( user );
    sprint( buf, "AUTH LOGIN %s\r\n", sstr( data ) );
    rc = smtp_cmd( smtp, sstr( buf ), 334, 0 );
    if( !rc )
    {
        sdel( buf );
        sdel( data );
        return 0;
    }
    sdel( data );
    data = base64_sencode( password );
    sprint( buf, "%s\r\n", sstr( data ) );
    rc = smtp_cmd( smtp, sstr( buf ), 235, 0 );
    sdel( buf );
    sdel( data );
    return rc;
}

int smtp_AUTH_PLAIN( KSmtp smtp, const char * user, const char * password )
{
    string buf;
    string data;
    int rc = smtp_cmd( smtp, "AUTH PLAIN\r\n", 334, 0 );
    if( !rc ) return 0;
    buf = snew();
    sprint( buf, "%c%s%c%s", '\0', user, '\0', password );
    data = base64_encode( sstr( buf ), slen( buf ) );
    sprint( buf, "%s\r\n", sstr( data ) );
    rc = smtp_cmd( smtp, sstr( data ), 235, 0 );
    sdel( data );
    sdel( buf );
    return rc;
}

int smtp_OpenSession( KSmtp smtp, const char * host, int port,
        int tls )
{
    if( !knet_connect( &smtp->sd, host, port ) )
    {
        sprint( smtp->error, "Could not connect to %s:%d (%s)", host, port,
                knet_error( &smtp->sd ) );
        return 0;
    }

    if( !smtp_cmd( smtp, NULL, 220, 0 ) ) return 0;
    if( !smtp_ehlo( smtp ) )
    {
        if( !smtp_cmd( smtp, "RSET\r\n", 250, 0 ) ) return 0;
        if( !smtp_helo( smtp ) ) return 0;
    }

    if( tls )
    {
        if( !smtp_cmd( smtp, "STARTTLS\r\n", 220, 0 )
                || !knet_init_ssl( &smtp->sd, TLSv1_client_method() )
                || !smtp_ehlo( smtp ) )
        {
            sprint( smtp->error, "Could not initialize TLS for %s:%d (%s)",
                    host, port, knet_error( &smtp->sd ) );
            return 0;
        }
    }
    return 1;
}

void smtp_CloseSession( KSmtp smtp )
{
    smtp_cmd( smtp, "QUIT\r\n", 221, 0 );
    knet_disconnect( &smtp->sd );
}

KSmtp smtp_Create( int timeout, const char * node, int verbose )
{
    KSmtp smtp = Calloc( sizeof(struct _KSmtp), 1 );
    if( !smtp ) return NULL;
    smtp->verbose = verbose;

    if( !node )
    {
        if( gethostname( smtp->node, sizeof(smtp->node) - 1 ) < 0 )
        {
            strncpy( smtp->node, "localhost", sizeof(smtp->node) - 1 );
        }
    }
    else
    {
        strncpy( smtp->node, node, sizeof(smtp->node) - 1 );
    }
    smtp->current = snew();
    smtp->error = snew();
    if( !smtp->error || !smtp->current )
    {
        sdel( smtp->error );
        sdel( smtp->current );
        Free( smtp );
        smtp = NULL;
    }
    knet_create_sd( &smtp->sd, timeout );
    return smtp;
}

void smtp_Destroy( KSmtp smtp )
{
    sdel( smtp->error );
    sdel( smtp->current );
    Free( smtp );
}

