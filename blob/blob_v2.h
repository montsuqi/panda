/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan & JMA (Japan Medical Association).

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
