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

#ifndef	_INC_BLOB_V1_H
#define	_INC_BLOB_V1_H

#define	BLOB_V1_HEADER		"PNB1"
#if	BLOB_VERSION == 1

#define	BLOB_OPEN_CLOSE		0x00
#define	BLOB_OPEN_CREATE	0x01
#define	BLOB_OPEN_READ		0x02
#define	BLOB_OPEN_WRITE		0x04
#define	BLOB_OPEN_APPEND	0x08

#define	BLOB_Space			BLOB_V1_Space
#define	BLOB_State			BLOB_V1_State

#define	InitBLOB			InitBLOB_V1
#define	FinishBLOB			FinishBLOB_V1
#define	ConnectBLOB			ConnectBLOB_V1
#define	DisConnectBLOB		DisConnectBLOB_V1
#define	StartBLOB			StartBLOB_V1
#define	CommitBLOB			CommitBLOB_V1
#define	AbortBLOB			AbortBLOB_V1

#define	NewBLOB				NewBLOB_V1
#define	OpenBLOB			OpenBLOB_V1
#define	DestroyBLOB			DestroyBLOB_V1
#define	CloseBLOB			CloseBLOB_V1
#define	WriteBLOB			WriteBLOB_V1
#define	ReadBLOB			ReadBLOB_V1
#endif

typedef	struct _BLOB_Space	{
	char			*space;
	FILE			*fp;
	MonObjectType	oid;
	GHashTable	*table;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	BLOB_V1_Space;

typedef	struct {
	char			magic[SIZE_BLOB_HEADER];
	MonObjectType	oid;
}	BLOB_V1_Header;

typedef	struct	_BLOB_V1_State {
	BLOB_V1_Space	*blob;
}	BLOB_V1_State;

extern	BLOB_V1_Space	*InitBLOB_V1(char *space);
extern	void			FinishBLOB_V1(BLOB_V1_Space *blob);
extern	BLOB_V1_State	*ConnectBLOB_V1(BLOB_V1_Space *blob);
extern	void			DisConnectBLOB_V1(BLOB_V1_State *state);
extern	Bool			StartBLOB_V1(BLOB_V1_State *state);
extern	Bool			CommitBLOB_V1(BLOB_V1_State *state);
extern	Bool			AbortBLOB_V1(BLOB_V1_State *state);

extern	MonObjectType	NewBLOB_V1(BLOB_V1_State *state, int mode);
extern	Bool			OpenBLOB_V1(BLOB_V1_State *state, MonObjectType obj, int mode);
extern	Bool			DestroyBLOB_V1(BLOB_V1_State *state, MonObjectType obj);
extern	Bool			CloseBLOB_V1(BLOB_V1_State *state, MonObjectType obj);
extern	int	WriteBLOB_V1(BLOB_V1_State *state, MonObjectType obj, byte *buff, size_t size);
extern	int	ReadBLOB_V1(BLOB_V1_State *state, MonObjectType obj, byte *buff, size_t size);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
