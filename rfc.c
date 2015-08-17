/*
 * rfc.c, part of "knet" project.
 *
 *  Created on: 05.06.2015, 23:00
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#include "rfc.h"
#include <stdlib.h>
#include <string.h>

char * rfc1123_date( char * buf, time_t t )
{
    static const char * DAY_NAMES[] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char * MONTH_NAMES[] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
            "Nov", "Dec" };
    time_t tt;
    struct tm * gt;

    if( !buf )
    {
        buf = Malloc( 32 );
        if( !buf ) return NULL;
    }

    tt = t ? t : time( NULL );
    gt = gmtime( &tt );
    strftime( buf, 31, "---, %d --- %Y %H:%M:%S GMT", gt );
    memcpy( buf, DAY_NAMES[gt->tm_wday], 3 );
    memcpy( buf + 8, MONTH_NAMES[gt->tm_mon], 3 );

    return buf;
}
