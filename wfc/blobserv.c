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
#include	"blobserv.h"
#include	"debug.h"

extern	void
PassiveBLOB(
	NETFILE		*fp,
	BLOB_Space	*blob)
{
	MonObjectType	obj;
	int				mode;
	size_t			size;
	ssize_t			ssize;
	byte			*buff;

ENTER_FUNC;
	switch	(RecvPacketClass(fp)) {
	  case	BLOB_CREATE:
		dbgmsg("BLOB_CREATE");
		mode = RecvInt(fp);			ON_IO_ERROR(fp,badio);
		if		(  ( obj = NewBLOB(blob,mode) )  !=  GL_OBJ_NULL  ) {
			SendPacketClass(fp,BLOB_OK);	ON_IO_ERROR(fp,badio);
			SendObject(fp,obj);				ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);	ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_OPEN:
		dbgmsg("BLOB_OPEN");
		mode = RecvInt(fp);			ON_IO_ERROR(fp,badio);
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  OpenBLOB(blob,obj,mode)  >=  0  ) {
			SendPacketClass(fp,BLOB_OK);	ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);	ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_WRITE:
		dbgmsg("BLOB_WRITE");
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  ( size = RecvLength(fp) )  >  0  ) {
			ON_IO_ERROR(fp,badio);
			buff = xmalloc(size);
			Recv(fp,buff,size);			ON_IO_ERROR(fp,badio);
			size = WriteBLOB(blob,obj,buff,size);
			xfree(buff);
		}
		SendLength(fp,size);
		break;
	  case	BLOB_READ:
		dbgmsg("BLOB_READ");
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  ( size = RecvLength(fp) )  >  0  ) {
			ON_IO_ERROR(fp,badio);
			buff = xmalloc(size);
			size = ReadBLOB(blob,obj,buff,size);
			Send(fp,buff,size);						ON_IO_ERROR(fp,badio);
			xfree(buff);
		}
		SendLength(fp,size);		ON_IO_ERROR(fp,badio);
		break;
	  case	BLOB_EXPORT:
		dbgmsg("BLOB_EXPORT");
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  ( ssize = OpenBLOB(blob,obj,BLOB_OPEN_READ) )  >=  0  ) {
			SendPacketClass(fp,BLOB_OK);			ON_IO_ERROR(fp,badio);
			SendLength(fp,ssize);					ON_IO_ERROR(fp,badio);
			buff = xmalloc((  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize);
			while	(  ssize  >  0  ) {
				size = (  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize;
				ReadBLOB(blob,obj,buff,size);		ON_IO_ERROR(fp,badio);
				Send(fp,buff,size);					ON_IO_ERROR(fp,badio);
				ssize -= size;
			}
			CloseBLOB(blob,obj);
			xfree(buff);
		} else {
			SendPacketClass(fp,BLOB_NOT);			ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_IMPORT:
		dbgmsg("BLOB_IMPORT");
		obj = GL_OBJ_NULL;
		if		(  ( obj = NewBLOB(blob,BLOB_OPEN_WRITE) )  !=  GL_OBJ_NULL  ) {
			SendPacketClass(fp,BLOB_OK);			ON_IO_ERROR(fp,badio);
			SendObject(fp,obj);						ON_IO_ERROR(fp,badio);
			ssize = RecvLength(fp);
			buff = xmalloc((  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize);
			while	(  ssize  >  0  ) {
				size = (  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize;
				Recv(fp,buff,size);					ON_IO_ERROR(fp,badio);
				WriteBLOB(blob,obj,buff,size);	
				ssize -= size;
			}
			CloseBLOB(blob,obj);
			xfree(buff);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_SAVE:
		dbgmsg("BLOB_SAVE");
		obj = RecvObject(fp);				ON_IO_ERROR(fp,badio);
		if		(  OpenBLOB(blob,obj,BLOB_OPEN_WRITE)  >=  0  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
			ssize = RecvLength(fp);				ON_IO_ERROR(fp,badio);
			buff = xmalloc((  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize);
			while	(  ssize  >  0  ) {
				size = (  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize;
				Recv(fp,buff,size);					ON_IO_ERROR(fp,badio);
				WriteBLOB(blob,obj,buff,size);
				ssize -= size;
			}
			CloseBLOB(blob,obj);
			xfree(buff);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_CHECK:
		dbgmsg("BLOB_CHECK");
		obj = RecvObject(fp);				ON_IO_ERROR(fp,badio);
		if		(  OpenBLOB(blob,obj,BLOB_OPEN_READ)  >=  0  ) {
			SendPacketClass(fp,BLOB_OK);			ON_IO_ERROR(fp,badio);
			CloseBLOB(blob,obj);
		} else {
			SendPacketClass(fp,BLOB_NOT);			ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_CLOSE:
		dbgmsg("BLOB_CLOSE");
		obj = RecvObject(fp);				ON_IO_ERROR(fp,badio);
		if		(  CloseBLOB(blob,obj)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_DESTROY:
		dbgmsg("BLOB_DESTROY");
		obj = RecvObject(fp);				ON_IO_ERROR(fp,badio);
		if		(  DestroyBLOB(blob,obj)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  default:
		break;
	}
  badio:
LEAVE_FUNC;
}

