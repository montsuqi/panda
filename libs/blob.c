/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"
#include	"blob.h"
#include	"message.h"
#include	"debug.h"

static	guint
IdHash(
	uint64_t	*key)
{
	guint	ret;

	ret = (guint)*key;
	return	(ret);
}

static	gint
IdCompare(
	uint64_t	*o1,
	uint64_t	*o2)
{
	int		check;

	if ((o1 != NULL) && (o2!=NULL)) {
		check = *o1 - *o2;
	} else {
		check = 1;
	}
	return	(check == 0);
}

static	GHashTable	*
NewLLHash(void)
{
	return	(g_hash_table_new((GHashFunc)IdHash,(GCompareFunc)IdCompare));
}

typedef	struct {
	NETFILE	*fp;
	MonObjectType	oid;
	struct	_BLOB_Space	*blob;
}	BLOB_Entry;

static	size_t
OpenEntry(
	BLOB_Entry	*ent,
	int		mode)
{
	char	longname[SIZE_LONGNAME+1];
	int		flag;
	int		fd;
	struct	stat	sb;
	size_t	size;

ENTER_FUNC;
	snprintf(longname,SIZE_LONGNAME+1,"%s/%d",ent->blob->space,(int)ent->oid);
	dbgprintf("%s",longname);
	if ((mode & BLOB_OPEN_WRITE) != 0 ) {
		dbgmsg("mode blob open write");
		flag = O_APPEND | O_TRUNC;
		if ((mode & BLOB_OPEN_READ) != 0) {
			flag |= O_RDWR;
		} else {
			flag |= O_WRONLY;
		}
	} else {
		dbgmsg("mode blob open read");
		flag = ((mode & BLOB_OPEN_READ) != 0) ? O_RDONLY : 0;
	}

	if ((mode & BLOB_OPEN_CREATE) != 0) {
		dbgmsg("mode blob open create");
		flag |= O_CREAT;
		fd = open(longname,flag,0600);
	} else {
		dbgmsg("open read or write mode");
		fd = open(longname,flag);
	}
	if (fd >= 0) {
		fstat(fd,&sb);
		ent->fp = FileToNet(fd);
		size = sb.st_size;
	} else {
		ent->fp = NULL;
		size = 0;
	}
LEAVE_FUNC;
	return size;
}

static gboolean
RemoveKeyTable(
	gpointer key, 
	gpointer value,
	gpointer data) 
{
	if (data == value) {
		xfree(key);
		return TRUE;
	}
	return FALSE;
}

static	void
DestroyEntry(
	BLOB_Entry	*ent)
{
	char	command[SIZE_LONGNAME+1];

ENTER_FUNC;
	if (ent->fp != NULL) {
		CloseNet(ent->fp);
	}
	if (ent->oid != GL_OBJ_NULL) {
		g_hash_table_remove(ent->blob->oid_table,(gpointer)&ent->oid);
		g_hash_table_foreach_remove(ent->blob->key_table,RemoveKeyTable,ent);
		snprintf(command,SIZE_LONGNAME+1,"rm -f %s/%d",
			ent->blob->space,(int)ent->oid);
		system(command);
	}
LEAVE_FUNC;
	xfree(ent);
}	

extern	BLOB_State	*
ConnectBLOB(
	BLOB_Space	*space)
{
	BLOB_State	*state;

	state = New(BLOB_State);
	state->blob = space;
	InitLock(state);
	return	(state);
}

extern	void
DisConnectBLOB(
	BLOB_State	*state)
{
	DestroyLock(state);
	xfree(state);
}

extern	Bool
StartBLOB(
	BLOB_State	*state)
{
	return	(TRUE);
}

extern	Bool
CommitBLOB(
	BLOB_State	*state)
{
	return	(TRUE);
}

extern	Bool
AbortBLOB(
	BLOB_State	*state)
{
	return	(TRUE);
}

extern	MonObjectType
NewBLOB(
	BLOB_State	*state,
	int				mode)
{
	BLOB_Entry	*ent;
	MonObjectType	obj;
	BLOB_Header	head;

ENTER_FUNC;
	obj = state->blob->oid;
	state->blob->oid ++;
	memcpy(head.magic,BLOB_HEADER,SIZE_BLOB_HEADER);
	head.oid = state->blob->oid;
	rewind(state->blob->fp);
	fwrite(&head,sizeof(head),1,state->blob->fp);
	fflush(state->blob->fp);
	fsync(fileno(state->blob->fp));

	ent = New(BLOB_Entry);
	ent->oid = obj;
	g_hash_table_insert(state->blob->oid_table,(gpointer)&ent->oid,ent);
	ent->blob = state->blob;

	mode |=  BLOB_OPEN_CREATE;
	OpenEntry(ent,mode);
	if (ent->fp == NULL) {
		DestroyEntry(ent);
		obj = GL_OBJ_NULL;
	}
LEAVE_FUNC;
	return	(obj);
}

extern	BLOB_Space	*
InitBLOB(
	char	*space)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	char	buff[SIZE_BUFF];
	BLOB_Space	*blob;
	BLOB_Header	head;

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s/pid",space);
	if ((fp = fopen(name,"r")) != NULL) {
		if (fgets(buff,SIZE_BUFF,fp) != NULL) {
			int pid = atoi(buff);
			if ((getpid() != pid) && (kill(pid, 0) == 0 || errno == EPERM)) {
				Error("blob: another process uses libpandablob. (%d)\n",
					atoi(buff));
			}
		}
		fclose(fp);
	} else {
		if ((fp = Fopen(name,"w")) != NULL) {
			fprintf(fp,"%d\n",getpid());
			fclose(fp);
		} else {
			Error("blob: can not make lock file(directory not writable?)");
		}
	}

	snprintf(name,SIZE_LONGNAME+1,"%s/pnb",space);
	if ((fp = Fopen(name,"r")) == NULL) {
		if ((fp = Fopen(name,"w")) != NULL) {
			memcpy(head.magic,BLOB_HEADER,SIZE_BLOB_HEADER);
			head.oid = 1;
			fwrite(&head,sizeof(head),1,fp);
			fclose(fp);
		} else {
			Error("blob: can not open BLOB space(disk full?)");
		}
	} else {
		fclose(fp);
	}
	if ((fp = fopen(name,"r+")) != NULL) {
		fread(&head,sizeof(head),1,fp);
		if (memcmp(head.magic,BLOB_HEADER,SIZE_BLOB_HEADER) != 0) {
			Error("blob: version mismatch");
		}
		fchmod(fileno(fp),0600);
		blob = New(BLOB_Space);
		blob->space = StrDup(space);
		blob->fp = fp;
		blob->oid = head.oid;
		blob->oid_table = NewLLHash();
		blob->key_table = NewNameHash();
	} else {
		blob = NULL;
	}
LEAVE_FUNC;
	return	(blob);
}

extern	void
FinishBLOB(
	BLOB_Space	*blob)
{
	char	name[SIZE_LONGNAME+1];

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s/pid",blob->space);
	unlink(name);

	xfree(blob->space);
	xfree(blob);
LEAVE_FUNC;
}

extern	ssize_t
OpenBLOB(
	BLOB_State	*state,
	MonObjectType	obj,
	int				mode)
{
	BLOB_Entry	*ent;
	ssize_t		ret;

ENTER_FUNC;
	ret = -1;
	if (obj != GL_OBJ_NULL) {
		ent = g_hash_table_lookup(state->blob->oid_table,(gpointer)&obj);
		if (ent == NULL) {
			ent = New(BLOB_Entry);
			ent->oid = obj;
			g_hash_table_insert(state->blob->oid_table,(gpointer)&ent->oid,ent);
			ent->blob = state->blob;
		}
		ret = OpenEntry(ent,mode);
		if		(  ent->fp  ==  NULL  ) {
			DestroyEntry(ent);
			ret = -1;
		}
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
CloseBLOB(
	BLOB_State	*state,
	MonObjectType	obj)
{
	BLOB_Entry	*ent;
	Bool		ret;

ENTER_FUNC;
	ent = g_hash_table_lookup(state->blob->oid_table,(gpointer)&obj);
	if (ent != NULL) {
		if (ent->fp != NULL) {
			CloseNet(ent->fp);
		}
		ret = TRUE;
		ent->fp = NULL;
	} else {
dbgmsg("CloseBLOB not found");
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
DestroyBLOB(
	BLOB_State	*state,
	MonObjectType	obj)
{
	BLOB_Entry	*ent;
	Bool			rc;
ENTER_FUNC;
	ent = g_hash_table_lookup(state->blob->oid_table,(gpointer)&obj);
	if (ent != NULL) {
		DestroyEntry(ent);
		rc = TRUE;
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
}

extern	int
WriteBLOB(
	BLOB_State	*state,
	MonObjectType	obj,
	unsigned char			*buff,
	size_t			size)
{
	BLOB_Entry	*ent;
	int			ret;

ENTER_FUNC;
	ent = g_hash_table_lookup(state->blob->oid_table,(gpointer)&obj);
	if (ent != NULL) {
		if (ent->fp != NULL) {
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

extern	unsigned char *
ReadBLOB(
	BLOB_State	*state,
	MonObjectType	obj,
	size_t			*size)
{
	BLOB_Entry	*ent;
	struct stat		st;
	unsigned char			*buff;

ENTER_FUNC;
	buff = NULL;
	*size = 0;
	ent = g_hash_table_lookup(state->blob->oid_table,(gpointer)&obj);
	if (ent != NULL) {
		if(ent->fp != NULL && !fstat(ent->fp->fd, &st)) {
			*size = st.st_size;
			buff = xmalloc(*size);
			Recv(ent->fp,buff,*size);
		}
	} else {
		dbgprintf("ent not found oid:%d\n", (int)obj);
	}
LEAVE_FUNC;
	return	buff;
}

