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

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"enum.h"
#include	"sysdatacom.h"
#include	"blob.h"
#include	"blobserv.h"
#include	"dbgroup.h"
#include	"bytea.h"
#include	"dbutils.h"
#include	"monsys.h"
#include	"debug.h"

#define BLOBEXPIRE 2

static DBG_Struct	*dbg;

static	void
blob_import(
	MonObjectType	obj,
	unsigned char	*buff,
	size_t			size)
{
	monblob_struct *monblob;
	ValueStruct *value = NULL;
	char	longname[SIZE_LONGNAME+1];

	monblob = NewMonblob_struct();
	monblob->blobid = (int)obj;
	snprintf(longname,SIZE_LONGNAME,"blob-%d",(int)obj);
	monblob->filename = StrDup(longname);
	monblob->size = size;
	timestamp(monblob->importtime, sizeof(monblob->importtime));
	value = escape_bytea(dbg, buff, size);
	monblob->bytea = ValueToString(value,NULL);
	monblob->bytea_len = strlen(monblob->bytea);
	monblob_insert(dbg, monblob);
}

static	ValueStruct *
blob_export(
	MonObjectType	obj)
{
	char	*sql;
	size_t	sql_len = SIZE_SQL;
	ValueStruct	*ret, *retval;

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT file_data FROM monblob WHERE blobid = %d AND now() < importtime + CAST('%d days' AS INTERVAL);", (int)obj, BLOBEXPIRE);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);

	if (!ret) {
		fprintf(stderr,"ERROR: [%d] is not registered\n", (int)obj);
		return NULL;
	}
	retval = unescape_bytea(dbg, GetItemLongName(ret,"file_data"));
	return retval;
}

extern	void
InitServeBLOB()
{
	dbg = GetDBG_monsys();
	dbg->dbt = NewNameHash();
	if (OpenDB(dbg) != MCP_OK ) {
		exit(1);
	}
}

extern	void
ServeBLOB(
	NETFILE		*fp,
	BLOB_State	*blob)
{
	MonObjectType	obj;
	int				mode;
	size_t			size;
	ssize_t			ssize;
	unsigned char	*buff;
	ValueStruct *value;

ENTER_FUNC;
	LockWrite(blob);
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
				SendLength(fp,size);	ON_IO_ERROR(fp,badio);
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
			buff = ReadBLOB(blob,obj,&size);
			CloseBLOB(blob,obj);
			DestroyBLOB(blob,obj);
			SendLength(fp,size);				ON_IO_ERROR(fp,badio);
			Send(fp,buff,size);					ON_IO_ERROR(fp,badio);
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
			value = blob_export(obj);
			SendLength(fp,ValueByteLength(value));		ON_IO_ERROR(fp,badio);
			Send(fp, ValueByte(value),ValueByteLength(value));
			FreeValueStruct(value);
			CloseBLOB(blob,obj);
			DestroyBLOB(blob,obj);
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
			ssize = RecvLength(fp);					ON_IO_ERROR(fp,badio);
			if (ssize > 0) {
				buff = xmalloc(ssize);
				Recv(fp,buff,ssize);					ON_IO_ERROR(fp,badio);
				blob_import(obj, buff, ssize);
				xfree(buff);
			}
			CloseBLOB(blob,obj);
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
	  default:
		break;
	}
  badio:
	UnLock(blob);
LEAVE_FUNC;
}

