/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
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
#include	"blob.h"
#include	"message.h"
#include	"debug.h"

#define	BLOB_FILE	"test"

#define	DIVIDE		40
#define	OB_NUMBER	40000
//#define	OB_NUMBER	20000
//#define	OB_NUMBER	1000
//#define	OB_NUMBER	20

#define	TEST_INIT
//#define	TEST_CREAT
//#define	TEST_DESTROY1
//#define	TEST_READWRITE1
#define	TEST_READWRITE2
#define	TEST_DESTROY2

extern	int
main(
	int		argc,
	char	**argv)
{
	BLOB_Space	*blob;
	BLOB_State	*state;
	MonObjectType	obj[OB_NUMBER];
	MonObjectType	lo[2];
	char	buff[SIZE_LONGNAME+1];
	int		i
		,	j
		,	k
		,	n
		,	size;

	InitMessage("testblob",NULL);
#ifdef	TEST_INIT
	blob = InitBLOB(BLOB_FILE);
	sleep(1);
	FinishBLOB(blob);
	sleep(1);
#endif
	blob = InitBLOB(BLOB_FILE);
	state = ConnectBLOB(blob);
	StartBLOB(state);
	printf("** new(1) **\n");
#ifdef	TEST_CREAT
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		obj[i] = NewBLOB(state,BLOB_OPEN_WRITE);
		printf("main oid = %lld\n",obj[i]);
	}
	printf("** close(1) **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		CloseBLOB(state,obj[i]);
	}
#endif
#ifdef	TEST_DESTROY1
	for	( i = OB_NUMBER / 4 ; i < ( OB_NUMBER * 3 ) / 4 ; i ++ ) {
		DestroyBLOB(state,obj[i]);
	}
	for	( i = OB_NUMBER / 4 ; i < ( OB_NUMBER * 3 ) / 4 ; i ++ ) {
		obj[i] = NewBLOB(state,BLOB_OPEN_WRITE);
		printf("main oid = %lld\n",obj[i]);
	}
#endif
#ifdef	TEST_READWRITE1
	printf("* test read/write(1) **\n");
	printf("** write(1) **\n");
	lo[0] = NewBLOB(state,BLOB_OPEN_WRITE);
	lo[1] = NewBLOB(state,BLOB_OPEN_WRITE);
	for	( i = 0 ; i < 200 ; i ++ ) {
		for	( j = 0 ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		WriteBLOB(state,lo[0],buff,64);
	}
	CloseBLOB(state,lo[0]);
	CloseBLOB(state,lo[1]);
	printf("** write(1) end **\n");

	printf("** read(1) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
	printf("** read(1) end **\n");
	printf("** read(2) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
	printf("** read(2) end **\n");
#endif
#ifdef	TEST_READWRITE2
	printf("* test read/write(2) **\n");
	printf("** open(1) **\n");
#if	1
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		obj[i] = NewBLOB(state,BLOB_OPEN_WRITE);
		printf("main oid = %lld\n",obj[i]);
	}
#else
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		OpenBLOB(state,obj[i],BLOB_OPEN_WRITE);
	}
#endif
	printf("** write(1) **\n");
	n = OB_NUMBER / DIVIDE ;
	for	( k = 0 ; k < DIVIDE ; k ++ ) {
		for	( i = n * k ; i < n * ( k + 1 ) ; i ++ ) {
			sprintf(buff,"add %d",i);
			for	( j = strlen(buff) ; j < 64 ; j ++ ) {
				buff[j] = (j+i)%64+32;
			}
			buff[j] = 0;
			printf("buff = [%s]\n",buff);
			WriteBLOB(state,obj[i],buff,strlen(buff)+1);
		}
		printf("** close(1) **\n");
		fprintf(stderr,"** close(1) **\n");
		for	( i = n * k ; i < n * ( k + 1 ) ; i ++ ) {
			CloseBLOB(state,obj[i]);
		}
	}
	printf("** open(2) **\n");
	fprintf(stderr,"** open(2) **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		OpenBLOB(state,obj[i],BLOB_OPEN_READ);
	}
	printf("** read(2) **\n");
	fprintf(stderr,"** read(2) **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		size = ReadBLOB(state,obj[i],buff,SIZE_LONGNAME+1);
		printf("%d = [%s]\n",size,buff);
	}
	printf("** close(2) **\n");
	fprintf(stderr,"** close(2) **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		CloseBLOB(state,obj[i]);
	}
	printf("* test read/write(2) end **\n");
#endif
#ifdef	TEST_READWRITE2
#ifdef	TEST_DESTROY2
	printf("* test destroy(2) *\n");
	fprintf(stderr,"* test destroy(2) *\n");
	printf("** destroy(1) **\n");
	fprintf(stderr,"** destroy(1) **\n");
	for	( i = OB_NUMBER / 4 ; i < ( OB_NUMBER * 3 ) / 4 ; i ++ ) {
		DestroyBLOB(state,obj[i]);
	}
	printf("** open(1) **\n");
	fprintf(stderr,"** open(1) **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		size = OpenBLOB(state,obj[i],BLOB_OPEN_READ);
		printf("%lld = %d\n",obj[i],size);
	}
	printf("** read **\n");
	fprintf(stderr,"** read **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		size = ReadBLOB(state,obj[i],buff,SIZE_LONGNAME+1);
		if		(  size  <  0  ) {
			*buff = 0;
		}
		printf("(%d:%lld) %d = [%s]\n",i,obj[i],size,buff);
	}
	printf("** read end **\n");
	fprintf(stderr,"** read end **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		CloseBLOB(state,obj[i]);
	}
	printf("* test destroy(2) end *\n");
	fprintf(stderr,"* test destroy(2) end *\n");
#endif
#endif
	CommitBLOB(state);
	DisConnectBLOB(state);
	FinishBLOB(blob);
	return	(0);
}
