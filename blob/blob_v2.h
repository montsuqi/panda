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

#ifndef	_INC_BLOB_V2_H
#define	_INC_BLOB_V2_H
#include	<stdint.h>

#if	BLOB_VERSION == 2
#define	BLOB_V2_HEADER		OSEKI_FILE_HEADER

#define	BLOB_OPEN_CLOSE		OSEKI_OPEN_CLOSE
#define	BLOB_OPEN_CREATE	OSEKI_OPEN_CREATE
#define	BLOB_OPEN_READ		OSEKI_OPEN_READ
#define	BLOB_OPEN_WRITE		OSEKI_OPEN_WRITE
#define	BLOB_OPEN_APPEND	OSEKI_OPEN_APPEND

#define	BLOB_Space			OsekiSpace
#define	BLOB_State			OsekiSession

#define	InitBLOB			InitOseki
#define	FinishBLOB			FinishOseki
#define	ConnectBLOB			ConnectOseki
#define	DisConnectBLOB		DisConnectOseki

#define	StartBLOB			OsekiTransactionStart
#define	CommitBLOB			OsekiTransactionCommit
#define	AbortBLOB			OsekiTransactionAbort

#define	NewBLOB				NewObject
#define	OpenBLOB			OpenObject
#define	DestroyBLOB			DestroyObject
#define	CloseBLOB			CloseObject
#define	WriteBLOB			WriteObject
#define	ReadBLOB			ReadObject
#endif

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
