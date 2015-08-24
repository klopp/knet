/*
 * kheader.h, part of "knet" project.
 *
 *  Created on: 23.08.2015, 17:09
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#ifndef KHEADER_H_
#define KHEADER_H_

#include "../klib/plist.h"
#include "../klib/slist.h"
#include "../stringlib/stringlib.h"

typedef struct _msg_Header
{
    char * name;
    List values;
}*msg_Header;

typedef struct _msg_Headers
{
    List mail;
    List text;

} *msg_Headers;

msg_Headers hdr_Create( void );
void hdr_Destroy( msg_Headers headers );

int hdr_AddTextHeader( msg_Headers headers, const char * key, const char * val );
int hdr_SetTextHeader( msg_Headers headers, const char * key, const char * val );

int hdr_AddMailHeader( msg_Headers headers, const char * key, const char * val );
int hdr_SetMailHeader( msg_Headers headers, const char * key, const char * val );

msg_Header hdr_GetHeader( msg_Headers headers, const char * key );
void hdr_ClearHeader( msg_Headers headers, const char * key );

string hdr_AsString( msg_Headers headers, const char * charset );

#endif /* KHEADER_H_ */
