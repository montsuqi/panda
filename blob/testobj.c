/*	OSEKI -- Object Store Engine Kernel Infrastructure

Copyright (C) 2004 Ogochan.

This module is part of OSEKI.

	OSEKI is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
OSEKI, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with OSEKI so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

/*
#define	DEBUG
#define	TRACE
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include    <sys/types.h>
#include	<sys/time.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"struct.h"
#include	"oseki.h"
#include	"message.h"
#include	"debug.h"

#define	OSEKI_FILE	"test"

//#define	OB_NUMBER	400000
//#define	OB_NUMBER	20000
//#define	OB_NUMBER	1000
//#define	OB_NUMBER	100
#define	OB_NUMBER	20

#define	TEST_SHORT

extern	int
main(
	int		argc,
	char	**argv)
{
	OsekiSpace		*space;
	OsekiSession	*ses;

	ObjectType	obj[OB_NUMBER];
	char	buff[SIZE_LONGNAME+1];
	int		i
		,	j
		,	size;

	InitMessage("testobj",NULL);
	space = InitOseki(OSEKI_FILE);
	ses = ConnectOseki(space);
	OsekiTransactionStart(ses);

	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		sprintf(buff,"add %d",i);
		memclear(buff,SIZE_LONGNAME+1);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		printf("buff = [%s]\n",buff);
		obj[i] = InitiateObject(ses,buff,SIZE_LONGNAME+1);
	}
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		size = GetObject(ses,obj[i],buff,SIZE_LONGNAME);
		buff[size] = 0;
		printf("(%d:%lld) %d = [%s]\n",i,obj[i],size,buff);
	}
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		memclear(buff,SIZE_LONGNAME+1);
		sprintf(buff,"put %d",i);
#if	0
		DestroyObject(ses,obj[i]);
		//		obj[i] = InitiateObject(ses,buff,strlen(buff)+1);
#else
		//		(void)PutObject(ses,obj[i],buff,strlen(buff)+1);
		(void)PutObject(ses,obj[i],buff,512);
#endif
	}
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		if		(  ( size = GetObject(ses,obj[i],buff,SIZE_LONGNAME) )  >  0  ) {
			buff[size] = 0;
			printf("(%d:%lld) %d = [%s]\n",i,obj[i],size,buff);
		}
	}
	OsekiTransactionCommit(ses);
	DisConnectOseki(ses);
	FinishOseki(space);
	return	(0);
}

