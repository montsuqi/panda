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
#include	<sys/stat.h>
#include	<glib.h>

#include	"types.h"

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"blob.h"
#include	"blobcom.h"
#include	"blobreq.h"
#include	"debug.h"

static	Bool
RequestBLOB(
	NETFILE	*fp,
	PacketClass	op)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,op);				ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:
LEAVE_FUNC;
	return	(rc);
}

extern	MonObjectType
RequestNewBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	int		mode)
{
	MonObjectType	obj;

ENTER_FUNC;
	obj = GL_OBJ_NULL;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_CREATE);		ON_IO_ERROR(fp,badio);
	SendInt(fp,mode);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		obj = RecvObject(fp);			ON_IO_ERROR(fp,badio);
	}
  badio:
LEAVE_FUNC;
	return	(obj);
}

extern	Bool
RequestOpenBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	int		mode,
	MonObjectType	obj)
{
	Bool	rc;
	
ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_OPEN);			ON_IO_ERROR(fp,badio);
	SendInt(fp,mode);					ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);				ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
RequestCloseBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj)
{
	Bool	rc;
	
ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_CLOSE);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

extern	size_t
RequestWriteBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj,
	byte	*buff,
	size_t	size)
{
	size_t	wrote;
	
ENTER_FUNC;
	wrote = 0;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_WRITE);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	SendLength(fp,size);				ON_IO_ERROR(fp,badio);
	if		(  size  >  0  ) {
		Send(fp,buff,size);					ON_IO_ERROR(fp,badio);
		wrote = RecvLength(fp);				ON_IO_ERROR(fp,badio);
	}
  badio:
LEAVE_FUNC;
	return	(wrote);
}

extern	size_t
RequestReadBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj,
	byte	*buff,
	size_t	size)
{
	size_t	red;
	
ENTER_FUNC;
	red = 0;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_READ);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	SendLength(fp,size);				ON_IO_ERROR(fp,badio);
	if		(  size  >  0  ) {
		Recv(fp,buff,size);					ON_IO_ERROR(fp,badio);
		red = RecvLength(fp);				ON_IO_ERROR(fp,badio);
	}
  badio:
LEAVE_FUNC;
	return	(red);
}

extern	Bool
RequestExportBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj,
	char			*fname)
{
	Bool	rc;
	char	buff[SIZE_BUFF];
	FILE	*fpf;
	size_t	size
		,	left;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_EXPORT);		ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		if		(  ( fpf = Fopen(fname,"w") )  !=  NULL  ) {
			left = RecvLength(fp);
			while	(  left  >  0  ) {
				size = (  left  >  SIZE_BUFF  ) ? SIZE_BUFF : left;
				Recv(fp,buff,size);			ON_IO_ERROR(fp,badio);
				fwrite(buff,size,1,fpf);
				left -= size;
			}
			fclose(fpf);
			chmod(fname,0644);
			rc = TRUE;
		}
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

extern	MonObjectType
RequestImportBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	char			*fname)
{
	MonObjectType	obj;
	char	buff[SIZE_BUFF];
	FILE	*fpf;
	struct	stat	sb;
	size_t	size
		,	left;

ENTER_FUNC;
	obj = GL_OBJ_NULL;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_IMPORT);		ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		obj = RecvObject(fp);				ON_IO_ERROR(fp,badio);
		if		(  ( fpf = fopen(fname,"r") )  !=  NULL  ) {
			fstat(fileno(fpf),&sb);
			left = sb.st_size;
			SendLength(fp,left);
			while	(  left  >  0  ) {
				size = (  left  >  SIZE_BUFF  ) ? SIZE_BUFF : left;
				fread(buff,size,1,fpf);
				Send(fp,buff,size);			ON_IO_ERROR(fp,badio);
				left -= size;
			}
			fclose(fpf);
		} else {
			SendLength(fp,0);
		}
	}
  badio:
LEAVE_FUNC;
	return	(obj);
}

extern	Bool
RequestSaveBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj,
	char			*fname)
{
	Bool	rc;
	char	buff[SIZE_BUFF];
	FILE	*fpf;
	struct	stat	sb;
	size_t	size
		,	left;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_SAVE);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		if		(  ( fpf = fopen(fname,"r") )  !=  NULL  ) {
			fstat(fileno(fpf),&sb);
			left = sb.st_size;
			SendLength(fp,left);
			while	(  left  >  0  ) {
				size = (  left  >  SIZE_BUFF  ) ? SIZE_BUFF : left;
				fread(buff,size,1,fpf);
				Send(fp,buff,size);			ON_IO_ERROR(fp,badio);
				left -= size;
			}
			fclose(fpf);
		} else {
			SendLength(fp,0);
		}
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
RequestCheckBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_CHECK);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
RequestDestroyBLOB(
	NETFILE	*fp,
	PacketClass		flag,
	MonObjectType	obj)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,flag);			ON_IO_ERROR(fp,badio);
	RequestBLOB(fp,BLOB_DESTROY);		ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  BLOB_OK  ) {
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

