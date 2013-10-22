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

#ifndef	_INC_BLOB_H
#define	_INC_BLOB_H

#include	"net.h"
#include	"lock.h"

#define	SIZE_BLOB_HEADER	4
#define	BLOB_HEADER			"PNB1"
#define	ObjectType			size_t

#define	BLOB_OPEN_CLOSE		0x00
#define	BLOB_OPEN_CREATE	0x01
#define	BLOB_OPEN_READ		0x02
#define	BLOB_OPEN_WRITE		0x04
#define	BLOB_OPEN_APPEND	0x08

typedef	struct _BLOB_Space	{
	char			*space;
	FILE			*fp;
	MonObjectType	oid;
	GHashTable		*oid_table;
	GHashTable		*key_table;
}	BLOB_Space;

typedef	struct {
	char			magic[SIZE_BLOB_HEADER];
	MonObjectType	oid;
}	BLOB_Header;

typedef	struct	_BLOB_State {
	BLOB_Space	*blob;
	LOCKOBJECT;
}	BLOB_State;

extern	BLOB_Space		*InitBLOB(char *space);
extern	void			FinishBLOB(BLOB_Space *blob);
extern	BLOB_State		*ConnectBLOB(BLOB_Space *blob);
extern	void			DisConnectBLOB(BLOB_State *state);
extern	Bool			StartBLOB(BLOB_State *state);
extern	Bool			CommitBLOB(BLOB_State *state);
extern	Bool			AbortBLOB(BLOB_State *state);

extern	MonObjectType	NewBLOB(BLOB_State *state, int mode);
extern	ssize_t			OpenBLOB(BLOB_State *state, MonObjectType obj, int mode);
extern	Bool			DestroyBLOB(BLOB_State *state, MonObjectType obj);
extern	Bool			CloseBLOB(BLOB_State *state, MonObjectType obj);
extern	int				WriteBLOB(BLOB_State *state, MonObjectType obj, unsigned char *buff, size_t size);
extern	unsigned char*	ReadBLOB(BLOB_State *state, MonObjectType obj, size_t *size);

#endif
