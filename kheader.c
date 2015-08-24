/*
 * kheader.c, part of "knet" project.
 *
 *  Created on: 23.08.2015, 17:12
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#include "kheader.h"
#include "kaddr.h"
#include "mime.h"
#include "../stringlib/b64.h"

#define ENCODED_BLK_SIZE    45

static string hdr_Encodeb64( const char * prefix, const char * value )
{
    string encoded = snew();
    size_t size = strlen( value );
    char * first = "";
    if( !encoded ) return NULL;

    while( size )
    {
        string b64;
        size_t to_encode = size > ENCODED_BLK_SIZE ? ENCODED_BLK_SIZE : size;
        b64 = base64_encode( value, to_encode );
        if( !b64 )
        {
            sdel( b64 );
            sdel( encoded );
            return NULL;
        }
        if( !xscatc( encoded, first, prefix, sstr( b64 ), "?=", NULL ) )
        {
            sdel( b64 );
            sdel( encoded );
            return NULL;
        }
        sdel( b64 );
        first = " ";
        size -= to_encode;
        value += to_encode;
        if( size )
        {
            if( !scatc( encoded, "\r\n" ) )
            {
                sdel( b64 );
                sdel( encoded );
                return NULL;
            }
        }
    }

    return encoded;
}

static void del_Header( void * ptr )
{
    if( ptr )
    {
        msg_Header header = (msg_Header)ptr;
        Free( header->name );
        ldestroy( header->values );
    }
}

msg_Headers hdr_Create( void )
{
    msg_Headers headers = Calloc( sizeof(struct _msg_Headers), 1 );
    if( headers )
    {
        headers->mail = lcreate( del_Header );
        headers->text = lcreate( del_Header );
    }
    return headers;
}

void hdr_Destroy( msg_Headers headers )
{
    ldestroy( headers->mail );
    ldestroy( headers->text );
    headers->text = headers->mail = NULL;
    Free( headers );
}

static msg_Header hdr_FindHeader( List headers, const char * key )
{
    msg_Header header = lfirst( headers );
    while( header )
    {
        if( !strcmp( header->name, key ) )
        {
            break;
        }
        header = lnext( headers );
    }
    return header;
}

int hdr_AddTextHeader( msg_Headers headers, const char * key,
        const char * value )
{
    msg_Header header = hdr_FindHeader( headers->text, key );

    if( !header )
    {
        header = Calloc( sizeof(struct _msg_Header), 1 );
        if( !header ) return 0;
        header->name = Strdup( key );
        if( !header->name )
        {
            Free( header );
            return 0;
        }
        header->values = slcreate();
        if( !header->values )
        {
            del_Header( header );
            return 0;
        }
        if( !sladd( header->values, value ) )
        {
            del_Header( header );
            return 0;
        }
    }

    return ladd( headers->text, header ) != NULL;
}

int hdr_AddMailHeader( msg_Headers headers, const char * key,
        const char * mail )
{
    msg_Header header = hdr_FindHeader( headers->text, key );

    if( !header )
    {
        Pair addr = addr_Create( mail );
        if( !addr ) return 0;

        header = Calloc( sizeof(struct _msg_Header), 1 );
        if( !header ) return 0;
        header->name = Strdup( key );
        if( !header->name )
        {
            pair_Delete( addr );
            Free( header );
            return 0;
        }
        header->values = plcreate();
        if( !header->values )
        {
            pair_Delete( addr );
            del_Header( header );
            return 0;
        }

        if( !pladd( header->values, A_NAME( addr ), A_EMAIL( addr ) ) )
        {
            pair_Delete( addr );
            del_Header( header );
            return 0;
        }
        pair_Delete( addr );
    }

    return ladd( headers->mail, header ) != NULL;
}

int hdr_SetMailHeader( msg_Headers headers, const char * key,
        const char * mail )
{
    msg_Header header = hdr_FindHeader( headers->mail, key );
    if( header ) lclear( header->values );
    return hdr_AddMailHeader( headers, key, mail );
}

int hdr_SetTextHeader( msg_Headers headers, const char * key, const char * val )
{
    msg_Header header = hdr_FindHeader( headers->text, key );
    if( header ) lclear( header->values );
    return hdr_AddTextHeader( headers, key, val );
}

void hdr_ClearHeader( msg_Headers headers, const char * key )
{
    msg_Header header = hdr_GetHeader( headers, key );
    if( header ) lclear( header->values );
}

msg_Header hdr_GetHeader( msg_Headers headers, const char * key )
{
    msg_Header header = hdr_FindHeader( headers->text, key );
    if( !header ) header = hdr_FindHeader( headers->mail, key );
    return header;
}

static int hdr_MakeTextHeader( msg_Header header, const char * cprefix,
        string out )
{
    char * value;
    string rc = snew();
    if( !rc ) return 0;

    value = slfirst( header->values );
    while( value )
    {
        if( isUsAscii( value ) )
        {
            if( !xscatc( rc, header->name, ": ", value, "\r\n", NULL ) )
            {
                sdel( rc );
                return 0;
            }
        }
        else
        {
            string b64 = hdr_Encodeb64( cprefix, value );
            if( !b64 ) return 0;
            if( !xscatc( rc, header->name, ": ", sstr( b64 ), "\r\n", NULL ) )
            {
                sdel( b64 );
                sdel( rc );
                return 0;
            }
            sdel( b64 );
        }
        value = slnext( header->values );
    }
    if( !scat( out, rc ) )
    {
        sdel( rc );
        return 0;
    }
    sdel( rc );
    return 1;
}

static int hdr_MakeTextHeaders( msg_Headers headers, const char * charset,
        string out )
{
    msg_Header header;
    char cprefix[32];
    string rc = snew();
    if( !rc ) return 0;

    if( !isUsAsciiCs( charset ) ) snprintf( cprefix, sizeof(cprefix) - 1,
            "=?%s?B?", charset );
    else *cprefix = 0;

    header = lfirst( headers->text );
    while( header )
    {
        if( !hdr_MakeTextHeader( header, cprefix, rc ) )
        {
            sdel( rc );
            return 0;
        }
        header = lnext( headers->text );
    }
    if( !scat( out, rc ) )
    {
        sdel( rc );
        return 0;
    }
    sdel( rc );
    return 1;
}

static string hdr_MakeEmail( const char * cprefix, Pair a )
{
    string buf = snew();
    if( !buf ) return NULL;

    if( A_NAME( a ) )
    {
        if( isUsAscii( A_NAME( a ) ) )
        {
            if( !sprint( buf, "%s <%s>", A_NAME( a ), A_EMAIL( a ) ) )
            {
                sdel( buf );
                return NULL;
            }
        }
        else
        {
            string b64 = hdr_Encodeb64( cprefix, A_NAME( a ) );
            if( !b64 )
            {
                sdel( buf );
                return NULL;
            }
            if( !sprint( buf, "%s <%s>", sstr( b64 ), A_EMAIL( a ) ) )
            {
                sdel( buf );
                sdel( b64 );
                return NULL;
            }
            sdel( b64 );
        }
    }
    else
    {
        if( !scatc( buf, A_EMAIL( a ) ) )
        {
            sdel( buf );
            return 0;
        }
    }
    return buf;
}

static int hdr_MakeMailHeader( msg_Header header, const char * cprefix,
        string out )
{
    Pair email;
    string rc;

    rc = snew();
    if( !rc ) return 0;

    email = plfirst( header->values );
    while( email )
    {
        string value = hdr_MakeEmail( cprefix, email );
        if( !value )
        {
            sdel( rc );
            return 0;
        }
        if( !scatc( rc, sstr( value ) ) )
        {
            sdel( rc );
            return 0;
        }
        email = plnext( header->values );
        if( !scatc( rc, email ? "," : "\r\n" ) )
        {
            sdel( rc );
            return 0;
        }
    }
    if( !scat( out, rc ) )
    {
        sdel( rc );
        return 0;
    }
    sdel( rc );
    return 1;
}

static int hdr_MakeMailHeaders( msg_Headers headers, const char * charset,
        string out )
{
    msg_Header header;
    char cprefix[32];
    string rc = snew();
    if( !rc ) return 0;

    if( !isUsAsciiCs( charset ) ) snprintf( cprefix, sizeof(cprefix) - 1,
            "=?%s?B?", charset );
    else *cprefix = 0;

    header = lfirst( headers->mail );
    while( header )
    {
        if( !xscatc( rc, header->name, ": ", NULL ) )
        {
            sdel( rc );
            return 0;
        }
        if( !hdr_MakeMailHeader( header, cprefix, rc ) )
        {
            sdel( rc );
            return 0;
        }
        header = lnext( headers->mail );
    }

    if( !scat( out, rc ) )
    {
        sdel( rc );
        return 0;
    }
    sdel( rc );
    return 1;
}

string hdr_AsString( msg_Headers headers, const char * charset )
{
    string out = snew();

    if( !out || !hdr_MakeMailHeaders( headers, charset, out )
            || !hdr_MakeTextHeaders( headers, charset, out ) )
    {
        sdel( out );
        return NULL;
    }
    return out;
}
