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
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<errno.h>

#include	"types.h"

#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"
#include	"blob_v1.h"
#include	"message.h"
#include	"debug.h"

#define	LockBLOB(blob)		pthread_mutex_lock(&(blob)->mutex)
#define	UnLockBLOB(blob)	pthread_mutex_unlock(&(blob)->mutex)
#define	ReleaseBLOB(blob)	pthread_cond_signal(&(blob)->cond)
#define	WaitBLOB(blod)		pthread_cond_wait(&(blob)->cond,&(blob)->mutex);

#define	BLOB_OPEN_CREATE	0x01
#define	BLOB_OPEN_APPEND	0x08

static	size_t
OpenEntry(
	BLOB_V1_Entry	*ent,
	int		mode)
{
	char	longname[SIZE_LONGNAME+1];
	int		flag;
	int		fd;
	struct	stat	sb;
	size_t	size;

ENTER_FUNC;
	snprintf(longname,SIZE_LONGNAME+1,"%s/%lld",ent->blob->space,ent->oid);

	if		(  ( mode & BLOB_OPEN_WRITE )  !=  0  ) {
#if	1
		flag = O_CREAT | O_APPEND | O_TRUNC;
#else
		if		(  ( mode & BLOB_OPEN_CREATE )  !=  0  ) {
			flag = O_CREAT;
		} else {
			flag = (  ( mode & BLOB_OPEN_APPEND )  !=  0  )  ? O_APPEND : O_TRUNC;
		}
#endif
		if		(  ( mode & BLOB_OPEN_READ )  !=  0  ) {
			flag |= O_RDWR;
		} else {
			flag |= O_WRONLY;
		}
	} else {
		flag = (  ( mode & BLOB_OPEN_READ )  !=  0  ) ? O_RDONLY : 0;
	}

	if		(  ( mode & BLOB_OPEN_CREATE )  !=  0  ) {
		fd = open(longname,flag,0600);
	} else {
		fd = open(longname,flag);
	}
	if		(  fd  >=  0  ) {
		fstat(fd,&sb);
		ent->fp = FileToNet(fd);
		size = sb.st_size;
	} else {
		ent->fp = NULL;
		size = 0;
	}
LEAVE_FUNC;
	return	(size);
}

static	void
DestroyEntry(
	BLOB_V1_Entry	*ent)
{
	char	command[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(  ent->fp  !=  NULL  ) {
		CloseNet(ent->fp);
	}
	if		(  ent->oid  !=  GL_OBJ_NULL  ) {
		LockBLOB(ent->blob);
		g_hash_table_remove(ent->blob->table,(gpointer)&ent->oid);
		snprintf(command,SIZE_LONGNAME+1,"rm -f %s/%lld",ent->blob->space,ent->oid);
		system(command);
		UnLockBLOB(ent->blob);
		ReleaseBLOB(ent->blob);
	}
LEAVE_FUNC;
	xfree(ent);
}	

extern	BLOB_V1_State	*
ConnectBLOB_V1(
	BLOB_V1_Space	*blob)
{
	BLOB_V1_State	*ret;

	ret = New(BLOB_V1_State);
	ret->blob = blob;
	return	(ret);
}

extern	void
DisConnectBLOB_V1(
	BLOB_V1_State	*state)
{
	xfree(state);
}

extern	Bool
StartBLOB_V1(
	BLOB_V1_State	*state)
{
	return	(TRUE);
}

extern	Bool
CommitBLOB_V1(
	BLOB_V1_State	*state)
{
	return	(TRUE);
}

extern	Bool
AbortBLOB_V1(
	BLOB_V1_State	*state)
{
	return	(TRUE);
}

extern	MonObjectType
NewBLOB_V1(
	BLOB_V1_State	*state,
	int				mode)
{
	BLOB_V1_Entry	*ent;
	MonObjectType	obj;
	BLOB_V1_Header	head;

ENTER_FUNC;
	LockBLOB(state->blob);
	obj = state->blob->oid;
	state->blob->oid ++;
	memcpy(head.magic,BLOB_V1_HEADER,SIZE_BLOB_HEADER);
	head.oid = state->blob->oid;
	rewind(state->blob->fp);
	fwrite(&head,sizeof(head),1,state->blob->fp);
	fflush(state->blob->fp);
	fsync(fileno(state->blob->fp));

	ent = New(BLOB_V1_Entry);
	ent->oid = obj;
	g_hash_table_insert(state->blob->table,(gpointer)&ent->oid,ent);
	UnLockBLOB(state->blob);
	ReleaseBLOB(state->blob);
	ent->blob = state->blob;

	mode |=  BLOB_OPEN_CREATE;
	OpenEntry(ent,mode);
	if		(  ent->fp  ==  NULL  ) {
		DestroyEntry(ent);
		obj = GL_OBJ_NULL;
	}
LEAVE_FUNC;
	return	(obj);
}

extern	BLOB_V1_Space	*
InitBLOB_V1(
	char	*space)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	char	buff[SIZE_BUFF];
	BLOB_V1_Space	*blob;
	BLOB_V1_Header	head;

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s/pid",space);
	if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
		if		(  fgets(buff,SIZE_BUFF,fp)  !=  NULL  ) {
			int pid = atoi(buff);
			if		(	(  getpid() != pid  )
					&&	(  kill(pid, 0) == 0 || errno == EPERM  ) ) {
				fprintf(	stderr,"another process uses libpandablob. (%d)\n",atoi(buff));
				exit(1);
			}
		}
		fclose(fp);
	} else {
		if		(  ( fp = Fopen(name,"w") )  !=  NULL  ) {
			fprintf(fp,"%d\n",getpid());
			fclose(fp);
		} else {
			fprintf(stderr,"can not make lock file(directory not writable?)\n");
			exit(1);
		}
	}

	snprintf(name,SIZE_LONGNAME+1,"%s/pnb",space);
	if		(  ( fp = Fopen(name,"r") )  ==  NULL  ) {
		if		(  ( fp = Fopen(name,"w") )  !=  NULL  ) {
			memcpy(head.magic,BLOB_V1_HEADER,SIZE_BLOB_HEADER);
			head.oid = 1;
			fwrite(&head,sizeof(head),1,fp);
			fclose(fp);
		} else {
			fprintf(stderr,"can not open BLOB space(disk full?)\n");
			exit(1);
		}
	} else {
		fclose(fp);
	}
	if		(  ( fp = fopen(name,"r+") )  !=  NULL  ) {
		fread(&head,sizeof(head),1,fp);
		if		(  memcmp(head.magic,BLOB_V1_HEADER,SIZE_BLOB_HEADER)  !=  0  ) {
			fprintf(stderr,"version mismatch\n");
			exit(1);
		}
		blob = New(BLOB_V1_Space);
		blob->space = StrDup(space);
		blob->fp = fp;
		blob->oid = head.oid;
		blob->table = NewLLHash();
		pthread_mutex_init(&blob->mutex,NULL);
		pthread_cond_init(&blob->cond,NULL);
	} else {
		blob = NULL;
	}

LEAVE_FUNC;
	return	(blob);
}

extern	void
FinishBLOB_V1(
	BLOB_V1_Space	*blob)
{
	char	name[SIZE_LONGNAME+1];

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s/pid",blob->space);
	unlink(name);

	xfree(blob->space);
	pthread_mutex_destroy(&blob->mutex);
	pthread_cond_destroy(&blob->cond);
	xfree(blob);
LEAVE_FUNC;
}

extern	ssize_t
OpenBLOB_V1(
	BLOB_V1_State	*state,
	MonObjectType	obj,
	int				mode)
{
	BLOB_V1_Entry	*ent;
	ssize_t		ret;

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(state->blob->table,(gpointer)&obj) )  ==  NULL  ) {
		LockBLOB(state->blob);
		ent = New(BLOB_V1_Entry);
		ent->oid = obj;
		g_hash_table_insert(state->blob->table,(gpointer)&ent->oid,ent);
		UnLockBLOB(state->blob);
		ReleaseBLOB(state->blob);
		ent->blob = state->blob;
	}
	ret = OpenEntry(ent,mode);
	if		(  ent->fp  ==  NULL  ) {
		DestroyEntry(ent);
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
CloseBLOB_V1(
	BLOB_V1_State	*state,
	MonObjectType	obj)
{
	BLOB_V1_Entry	*ent;
	Bool		ret;

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(state->blob->table,(gpointer)&obj) )
			   !=  NULL  ) {
		if		(  ent->fp  !=  NULL  ) {
			CloseNet(ent->fp);
		}
		ret = TRUE;
		ent->fp = NULL;
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
DestroyBLOB_V1(
	BLOB_V1_State	*state,
	MonObjectType	obj)
{
	BLOB_V1_Entry	*ent;
	Bool			rc;
ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(state->blob->table,(gpointer)&obj) )
			   !=  NULL  ) {
		DestroyEntry(ent);
		rc = TRUE;
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
}

extern	int
WriteBLOB_V1(
	BLOB_V1_State	*state,
	MonObjectType	obj,
	byte			*buff,
	size_t			size)
{
	BLOB_V1_Entry	*ent;
	int			ret;

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(state->blob->table,(gpointer)&obj) )
			   !=  NULL  ) {
		if		(  ent->fp  !=  NULL  ) {
			ret = Send(ent->fp,buff,size);
		} else {
			ret = -1;
		}
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	int
ReadBLOB_V1(
	BLOB_V1_State	*state,
	MonObjectType	obj,
	byte			*buff,
	size_t			size)
{
	BLOB_V1_Entry	*ent;
	int			ret;

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(state->blob->table,(gpointer)&obj) )
			   !=  NULL  ) {
		if		(  ent->fp  !=  NULL  ) {
			ret = Recv(ent->fp,buff,size);
		} else {
			ret = -1;
		}
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}
