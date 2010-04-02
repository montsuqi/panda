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

#ifndef	_INC_BLOB_V1_H
#define	_INC_BLOB_V1_H

#include "lock.h"

#define	BLOB_V1_HEADER		"PNB1"
#if	BLOB_VERSION == 1

#define	ObjectType			size_t

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
#define	LookupBLOB			LookupBLOB_V1
#define	DestroyBLOB			DestroyBLOB_V1
#define	CloseBLOB			CloseBLOB_V1
#define	WriteBLOB			WriteBLOB_V1
#define	ReadBLOB			ReadBLOB_V1
#define	RegisterBLOB		RegisterBLOB_V1
#define LookupBLOB			LookupBLOB_V1
#endif

typedef	struct _BLOB_Space	{
	char			*space;
	FILE			*fp;
	MonObjectType	oid;
	GHashTable		*oid_table;
	GHashTable		*key_table;
}	BLOB_V1_Space;

typedef	struct {
	char			magic[SIZE_BLOB_HEADER];
	MonObjectType	oid;
}	BLOB_V1_Header;

typedef	struct	_BLOB_V1_State {
	BLOB_V1_Space	*blob;
	LOCKOBJECT;
}	BLOB_V1_State;

extern	BLOB_V1_Space	*InitBLOB_V1(char *space);
extern	void			FinishBLOB_V1(BLOB_V1_Space *blob);
extern	BLOB_V1_State	*ConnectBLOB_V1(BLOB_V1_Space *blob);
extern	void			DisConnectBLOB_V1(BLOB_V1_State *state);
extern	Bool			StartBLOB_V1(BLOB_V1_State *state);
extern	Bool			CommitBLOB_V1(BLOB_V1_State *state);
extern	Bool			AbortBLOB_V1(BLOB_V1_State *state);

extern	MonObjectType	NewBLOB_V1(BLOB_V1_State *state, int mode);
extern	ssize_t			OpenBLOB_V1(BLOB_V1_State *state, MonObjectType obj, int mode);
extern	Bool			DestroyBLOB_V1(BLOB_V1_State *state, MonObjectType obj);
extern	Bool			CloseBLOB_V1(BLOB_V1_State *state, MonObjectType obj);
extern	int				WriteBLOB_V1(BLOB_V1_State *state, MonObjectType obj, unsigned char *buff, size_t size);
extern	unsigned char*			ReadBLOB_V1(BLOB_V1_State *state, MonObjectType obj, size_t *size);
extern	Bool			RegisterBLOB_V1(BLOB_V1_State *state, MonObjectType obj, char *key);
extern	MonObjectType	LookupBLOB_V1(BLOB_V1_State *state, char *k);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
