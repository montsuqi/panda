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

#ifndef	_INC_BLOB_H
#define	_INC_BLOB_H

typedef	struct {
	int		mode;
	NETFILE	*fp;
	MonObjectType	*oid;
	struct	_BLOB_Space	*blob;
}	BLOB_Entry;

typedef	struct _BLOB_Space	{
	char	*space;
	size_t	oid;
	GHashTable	*table;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	BLOB_Space;

extern	BLOB_Space	*InitBLOB(char *space);
extern	void		FinishBLOB(BLOB_Space *blob);
extern	Bool	NewBLOB(BLOB_Space *blob,MonObjectType *obj, int mode);
extern	Bool	OpenBLOB(BLOB_Space *blob, MonObjectType *obj, int mode);
extern	Bool	CloseBLOB(BLOB_Space *blob, MonObjectType *obj);
extern	int		WriteBLOB(BLOB_Space *blob, MonObjectType *obj, byte *buff, size_t size);
extern	int		ReadBLOB(BLOB_Space *blob, MonObjectType *obj, byte *buff, size_t size);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#define	BLOB_OPEN_CREATE	0x01
#define	BLOB_OPEN_READ		0x02
#define	BLOB_OPEN_WRITE		0x04
#define	BLOB_OPEN_APPEND	0x08

#endif
