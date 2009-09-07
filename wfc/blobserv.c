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
#include    <sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"blob.h"
#include	"blobserv.h"
#include	"blobcom.h"
#include	"debug.h"

static	pthread_mutex_t	mutex;

extern	void
InitBLOBServ()
{
ENTER_FUNC;
	pthread_mutex_init(&mutex,NULL);
LEAVE_FUNC;
}

extern	void
PassiveBLOB(
	NETFILE		*fp,
	BLOB_State	*blob)
{
	MonObjectType	obj;
	int				mode;
	size_t			size;
	ssize_t			ssize;
	byte			*buff;
	char			*str;

ENTER_FUNC;
	pthread_mutex_lock(&mutex);
	switch	(RecvPacketClass(fp)) {
	  case	BLOB_CREATE:
		dbgmsg("BLOB_CREATE");
		mode = RecvInt(fp);			ON_IO_ERROR(fp,badio);
		if		(  ( obj = NewBLOB(blob,mode) )  !=  GL_OBJ_NULL  ) {
			CloseBLOB(blob,obj);
			SendPacketClass(fp,BLOB_OK);	ON_IO_ERROR(fp,badio);
			SendObject(fp,obj);				ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);	ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_WRITE:
		dbgmsg("BLOB_WRITE");
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  OpenBLOB(blob,obj,BLOB_OPEN_WRITE)  >=  0  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
			if		(  ( size = RecvLength(fp) )  >  0  ) {
				ON_IO_ERROR(fp,badio);
				buff = xmalloc(size);
				Recv(fp,buff,size);			ON_IO_ERROR(fp,badio);
				size = WriteBLOB(blob,obj,buff,size);
				CloseBLOB(blob,obj);
				xfree(buff);
				SendLength(fp,size);
			}
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_READ:
		dbgmsg("BLOB_READ");
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  OpenBLOB(blob,obj,BLOB_OPEN_READ)  >=  0  ) {
		dbgmsg("   OpenBLOB end");
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
			ON_IO_ERROR(fp,badio);
			buff = ReadBLOB(blob,obj,&size);
			CloseBLOB(blob,obj);
			SendLength(fp,size);					ON_IO_ERROR(fp,badio);
			Send(fp,buff,size);						ON_IO_ERROR(fp,badio);
			Flush(fp);
			xfree(buff);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_EXPORT:
		dbgmsg("BLOB_EXPORT");
		obj = RecvObject(fp);		ON_IO_ERROR(fp,badio);
		if		(  ( ssize = OpenBLOB(blob,obj,BLOB_OPEN_READ) )  >=  0  ) {
			SendPacketClass(fp,BLOB_OK);			ON_IO_ERROR(fp,badio);
			SendLength(fp,ssize);					ON_IO_ERROR(fp,badio);
			while	(  ssize  >  0  ) {
				size = (  ssize  >  SIZE_BUFF  ) ? SIZE_BUFF : ssize;
				buff = ReadBLOB(blob,obj,&size);	ON_IO_ERROR(fp,badio);
				Send(fp,buff,size);					ON_IO_ERROR(fp,badio);
				xfree(buff);
				ssize -= size;
			}
			CloseBLOB(blob,obj);
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
	  case	BLOB_DESTROY:
		dbgmsg("BLOB_DESTROY");
		obj = RecvObject(fp);				ON_IO_ERROR(fp,badio);
		if		(  DestroyBLOB(blob,obj)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_START:
		if		(  StartBLOB(blob)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_COMMIT:
		if		(  CommitBLOB(blob)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
	  case	BLOB_ABORT:
		if		(  AbortBLOB(blob)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		break;
#if BLOB_VERSION == 1
	  case	BLOB_REGISTER:
		dbgmsg("BLOB_REGISTER");
		obj = RecvObject(fp);					ON_IO_ERROR(fp,badio);
		str = RecvStringNew(fp);				ON_IO_ERROR(fp,badio);
		if		(  RegisterBLOB(blob, obj, str)  ) {
			SendPacketClass(fp,BLOB_OK);		ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,BLOB_NOT);		ON_IO_ERROR(fp,badio);
		}
		xfree(str);
		break;
	  case	BLOB_LOOKUP:
		dbgmsg("BLOB_Lookup");
		str = RecvStringNew(fp);			ON_IO_ERROR(fp,badio);
		obj = LookupBLOB(blob, str);
		SendObject(fp, obj); 				ON_IO_ERROR(fp,badio);
		xfree(str);
		break;
#endif
	  default:
		break;
	}
  badio:
	pthread_mutex_unlock(&mutex);
LEAVE_FUNC;
}

