/*
PANDA -- a simple transaction monitor
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

#define	DIVIDE		1
//#define	OB_NUMBER	40000
//#define	OB_NUMBER	20000
//#define	OB_NUMBER	1000
#define	OB_NUMBER	20

#define	TEST_PACK
#if	BLOB_VERSION == 2
#define	TEST_LINER
#define	TEST_TREE
#endif

//#define	TEST_INIT
//#define	TEST_CREAT
//#define	TEST_DESTROY1
//#define	TEST_READWRITE1
//#define	TEST_SEEK1
//#define	TEST_SEEK2
//#define	TEST_SEEK3
//#define	TEST_READWRITE2
//#define	TEST_DESTROY2
//#define	TEST_TRANSACTION
//#if	BLOB_VERSION == 2
//#define	TEST_SHORT
//#endif

extern	int
main(
	int		argc,
	char	**argv)
{
	BLOB_Space	*blob;
	BLOB_State	*state;
	ObjectType	lo[3];
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
#ifdef	TEST_PACK
	printf("** write(1) **\n");
	lo[0] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_PACK);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[0],buff,64);
	}
	CloseBLOB(state,lo[0]);
	printf("** write(1) end **\n");
	printf("** read(1) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	i = 0;
	do {
		i ++;
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	printf("rc = %d\n",i);
	CloseBLOB(state,lo[0]);
	printf("** read(1) end **\n");
#endif
#ifdef	TEST_LINER
	printf("** write(2) **\n");
	lo[1] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_LINER);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[1],buff,64);
	}
	CloseBLOB(state,lo[1]);
	printf("** write(2) end **\n");
	printf("** read(2) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	i = 0;
	do {
		i ++;
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	printf("rc = %d\n",i);
	CloseBLOB(state,lo[1]);
	printf("** read(2) end **\n");
#endif
#ifdef	TEST_TREE
	printf("** write(3) **\n");
	lo[2] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_TREE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[2],buff,64);
	}
	CloseBLOB(state,lo[2]);
	printf("** write(3) end **\n");
	printf("** read(3) **\n");
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	i = 0;
	do {
		i ++;
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	printf("rc = %d\n",i);
	CloseBLOB(state,lo[2]);
	printf("** read(3) end **\n");
#endif
#endif

#ifdef	TEST_SEEK1
	printf("* test seek **\n");
	printf("** write(1) **\n");
#ifdef	TEST_PACK
	lo[0] = NewBLOB(state,BLOB_OPEN_WRITE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[0],buff,64);
	}
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	lo[1] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_LINER);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[1],buff,64);
	}
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	lo[2] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_TREE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[2],buff,64);
	}
	CloseBLOB(state,lo[2]);
#endif
	printf("** write(1) end **\n");
#ifdef	TEST_PACK
	printf("** read(1) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	printf("** read(2) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	printf("** read(3) **\n");
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[2]);
#endif
	printf("** all read end **\n");

	printf("** seek 64*4 **\n");
#ifdef	TEST_PACK
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	SeekObject(state,lo[0],64*4);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_LINER
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	SeekObject(state,lo[1],64*4);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_TREE
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	SeekObject(state,lo[2],64*4);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif

	printf("** seek 64*128 **\n");
#ifdef	TEST_PACK
	SeekObject(state,lo[0],64*128);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],64*128);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],64*128);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif

	printf("** seek 32*5 **\n");
#ifdef	TEST_PACK
	SeekObject(state,lo[0],32*5);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],32*5);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],32*5);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif

	printf("** seek 64*256 **\n");
#ifdef	TEST_PACK
	SeekObject(state,lo[0],64*256);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],64*256);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],64*256);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
#endif

	printf("** seek 64*511 **\n");
#ifdef	TEST_PACK
	SeekObject(state,lo[0],64*511);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],64*511);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],64*511);
	do {
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[2]);
	printf("** read(2) end **\n");
#endif
#endif
#ifdef	TEST_SEEK2
	printf("* test seek(2) **\n");
	printf("** write(1) **\n");
#ifdef	TEST_PACK
	lo[0] = NewBLOB(state,BLOB_OPEN_WRITE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[0],buff,64);
	}
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	lo[1] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_LINER);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[1],buff,64);
	}
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	lo[2] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_TREE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[2],buff,64);
	}
	CloseBLOB(state,lo[2]);
#endif
	printf("** write(1) end **\n");
#ifdef	TEST_PACK
	printf("** read(1) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	printf("** read(2) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	printf("** read(3) **\n");
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[2]);
#endif
	printf("** all read end **\n");

	printf("** write(2) **\n");
	printf("** seek 64*4**\n");
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '1';
	}
#ifdef	TEST_PACK
	OpenBLOB(state,lo[0],BLOB_OPEN_READ|BLOB_OPEN_WRITE);
	SeekObject(state,lo[0],64*4);
	size = WriteBLOB(state,lo[0],buff,64);
#endif
#ifdef	TEST_LINER
	OpenBLOB(state,lo[1],BLOB_OPEN_READ|BLOB_OPEN_WRITE);
	SeekObject(state,lo[1],64*4);
	size = WriteBLOB(state,lo[1],buff,64);
#endif
#ifdef	TEST_TREE
	OpenBLOB(state,lo[2],BLOB_OPEN_READ|BLOB_OPEN_WRITE);
	SeekObject(state,lo[2],64*4);
	size = WriteBLOB(state,lo[2],buff,64);
#endif

	printf("** seek 64*128 **\n");
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '2';
	}
#ifdef	TEST_PACK
	SeekObject(state,lo[0],64*128);
	size = WriteBLOB(state,lo[0],buff,64);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],64*128);
	size = WriteBLOB(state,lo[1],buff,64);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],64*128);
	size = WriteBLOB(state,lo[2],buff,64);
#endif

	printf("** seek 64*256 **\n");
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '3';
	}
#ifdef	TEST_PACK
	SeekObject(state,lo[0],64*256);
	size = WriteBLOB(state,lo[0],buff,64);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],64*256);
	size = WriteBLOB(state,lo[1],buff,64);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],64*256);
	size = WriteBLOB(state,lo[2],buff,64);
#endif

	printf("** seek 64*511 **\n");
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '4';
	}
#ifdef	TEST_PACK
	SeekObject(state,lo[0],64*511);
	size = WriteBLOB(state,lo[0],buff,64);
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	SeekObject(state,lo[1],64*511);
	size = WriteBLOB(state,lo[1],buff,64);
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	SeekObject(state,lo[2],64*511);
	size = WriteBLOB(state,lo[2],buff,64);
	CloseBLOB(state,lo[2]);
#endif
	printf("** write(2) end **\n");

#ifdef	TEST_PACK
	printf("** read(4) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	printf("** read(5) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	printf("** read(6) **\n");
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[2]);
#endif
#endif
#ifdef	TEST_SEEK3
	printf("* test seek **\n");
	printf("** write(1) **\n");
#ifdef	TEST_PACK
	lo[0] = NewBLOB(state,BLOB_OPEN_WRITE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[0],buff,64);
	}
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	lo[1] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_LINER);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[1],buff,64);
	}
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	lo[2] = NewBLOB(state,BLOB_OPEN_WRITE|OSEKI_ALLOC_TREE);
	for	( i = 0 ; i < 512 ; i ++ ) {
		sprintf(buff,"add %d",i);
		for	( j = strlen(buff) ; j < 64 ; j ++ ) {
			buff[j] = (j+i)%64+32;
		}
		buff[j] = 0;
		WriteBLOB(state,lo[2],buff,64);
	}
	CloseBLOB(state,lo[2]);
#endif
	printf("** write(1) end **\n");
#ifdef	TEST_PACK
	printf("** read(1) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	printf("** read(2) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	printf("** read(3) **\n");
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[2]);
#endif
	printf("** all read end **\n");

	printf("** write(2) **\n");
#ifdef	TEST_PACK
	OpenBLOB(state,lo[0],BLOB_OPEN_READ|BLOB_OPEN_WRITE);
	printf("** seek 64*511 **\n");
	SeekObject(state,lo[0],64*511);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '4';
	}
	SeekObject(state,lo[0],64*511);
	size = WriteBLOB(state,lo[0],buff,64);
#endif
#ifdef	TEST_LINER
	OpenBLOB(state,lo[1],BLOB_OPEN_READ|BLOB_OPEN_WRITE);
	printf("** seek 64*511 **\n");
	SeekObject(state,lo[1],64*511);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '4';
	}
	SeekObject(state,lo[1],64*511);
	size = WriteBLOB(state,lo[1],buff,64);
#endif
#ifdef	TEST_TREE
	OpenBLOB(state,lo[2],BLOB_OPEN_READ|BLOB_OPEN_WRITE);
	printf("** seek 64*511 **\n");
	SeekObject(state,lo[2],64*511);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '4';
	}
	SeekObject(state,lo[2],64*511);
	size = WriteBLOB(state,lo[2],buff,64);
#endif

#ifdef	TEST_PACK
	printf("** seek 64*128 **\n");
	SeekObject(state,lo[0],64*128);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '2';
	}
	SeekObject(state,lo[0],64*128);
	size = WriteBLOB(state,lo[0],buff,64);
#endif
#ifdef	TEST_LINER
	printf("** seek 64*128 **\n");
	SeekObject(state,lo[1],64*128);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '2';
	}
	SeekObject(state,lo[1],64*128);
	size = WriteBLOB(state,lo[1],buff,64);
#endif
#ifdef	TEST_TREE
	printf("** seek 64*128 **\n");
	SeekObject(state,lo[2],64*128);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '2';
	}
	SeekObject(state,lo[2],64*128);
	size = WriteBLOB(state,lo[2],buff,64);
#endif

#ifdef	TEST_PACK
	printf("** seek 64*4 **\n");
	SeekObject(state,lo[0],64*4);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '1';
	}
	SeekObject(state,lo[0],64*4);
	size = WriteBLOB(state,lo[0],buff,64);
#endif
#ifdef	TEST_LINER
	printf("** seek 64*4 **\n");
	SeekObject(state,lo[1],64*4);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '1';
	}
	SeekObject(state,lo[1],64*4);
	size = WriteBLOB(state,lo[1],buff,64);
#endif
#ifdef	TEST_TREE
	printf("** seek 64*4 **\n");
	SeekObject(state,lo[2],64*4);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '1';
	}
	SeekObject(state,lo[2],64*4);
	size = WriteBLOB(state,lo[2],buff,64);
#endif

#ifdef	TEST_PACK
	printf("** seek 64*256 **\n");
	SeekObject(state,lo[0],64*256);
	size = ReadBLOB(state,lo[0],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '3';
	}
	SeekObject(state,lo[0],64*256);
	size = WriteBLOB(state,lo[0],buff,64);
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	printf("** seek 64*256 **\n");
	SeekObject(state,lo[1],64*256);
	size = ReadBLOB(state,lo[1],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '3';
	}
	SeekObject(state,lo[1],64*256);
	size = WriteBLOB(state,lo[1],buff,64);
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	printf("** seek 64*256 **\n");
	SeekObject(state,lo[2],64*256);
	size = ReadBLOB(state,lo[2],buff,64);
	buff[size] = 0;
	printf("%d = [%s]\n",size,buff);
	for	( j = 0 ; j < 64 ; j ++ ) {
		buff[j] = '3';
	}
	SeekObject(state,lo[2],64*256);
	size = WriteBLOB(state,lo[2],buff,64);
	CloseBLOB(state,lo[2]);
#endif
	printf("** write(2) end **\n");
#ifdef	TEST_PACK
	printf("** read(3) **\n");
	OpenBLOB(state,lo[0],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[0],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[0]);
#endif
#ifdef	TEST_LINER
	printf("** read(4) **\n");
	OpenBLOB(state,lo[1],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[1],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[1]);
#endif
#ifdef	TEST_TREE
	printf("** read(5) **\n");
	OpenBLOB(state,lo[2],BLOB_OPEN_READ);
	do {
		size = ReadBLOB(state,lo[2],buff,64);
		buff[size] = 0;
		printf("%d = [%s]\n",size,buff);
	}	while	(  size  >  0  );
	CloseBLOB(state,lo[2]);
#endif
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
#ifdef	TEST_TRANSACTION
	StartBLOB(state);
	printf("** new(1) **\n");
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		obj[i] = NewBLOB(state,BLOB_OPEN_WRITE);
		printf("main oid = %lld\n",obj[i]);
	}
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
		fprintf(stderr,"** close(1) **\n");
		for	( i = n * k ; i < n * ( k + 1 ) ; i ++ ) {
			CloseBLOB(state,obj[i]);
		}
		printf("** close(1) **\n");
	}
	//AbortBLOB(state);
	sleep(2);
	CommitBLOB(state);
	StartBLOB(state);
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
	printf("* test transaction(1) **\n");
#endif
	DisConnectBLOB(state);
	FinishBLOB(blob);
	return	(0);
}
