/*
 * rfc.h, part of "knet" project.
 *
 *  Created on: 05.06.2015, 23:00
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#ifndef RFC_H_
#define RFC_H_

#include "../klib/config.h"
#include <time.h>

char * rfc1123_date( char * buf, time_t t );

#endif /* RFC_H_ */
