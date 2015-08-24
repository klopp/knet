/*
 * kaddr.h, part of "knet" project.
 *
 *  Created on: 20.05.2015, 01:15
 *      Author: Vsevolod Lutovinov <klopp@yandex.ru>
 */

#ifndef KADDR_H_
#define KADDR_H_

#include "../klib/plist.h"

#define A_EMAIL( pair ) (pair)->second
#define A_NAME( pair )  (pair)->first

Pair addr_Create( const char * src );

#endif /* KADDR_H_ */
