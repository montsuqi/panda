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

#define	BLOB_V1_Entry		BLOB_Entry
#define	BLOB_V1_Space		BLOB_Space
#define	InitBLOB_V1			InitBLOB
#define	FinishBLOB_V1		FinishBLOB
#define	NewBLOB_V1			NewBLOB
#define	OpenBLOB_V1			OpenBLOB
#define	DestroyBLOB_V1		DestroyBLOB
#define	CloseBLOB_V1		CloseBLOB
#define	WriteBLOB_V1		WriteBLOB
#define	ReadBLOB_V1			ReadBLOB

typedef	struct {
	int		mode;
	NETFILE	*fp;
	MonObjectType	oid;
	struct	_BLOB_Space	*blob;
}	BLOB_V1_Entry;

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

extern	BLOB_V1_Space	*InitBLOB_V1(char *space);
extern	void			FinishBLOB_V1(BLOB_V1_Space *blob);
extern	MonObjectType	NewBLOB_V1(BLOB_V1_Space *blob, int mode);
extern	Bool			OpenBLOB_V1(BLOB_V1_Space *blob, MonObjectType obj, int mode);
extern	Bool			DestroyBLOB_V1(BLOB_V1_Space *blob, MonObjectType obj);
extern	Bool			CloseBLOB_V1(BLOB_V1_Space *blob, MonObjectType obj);
extern	int	WriteBLOB_V1(BLOB_V1_Space *blob, MonObjectType obj, byte *buff, size_t size);
extern	int	ReadBLOB_V1(BLOB_V1_Space *blob, MonObjectType obj, byte *buff, size_t size);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
