/*	OSEKI -- Object Store Engine Kernel Infrastructure

Copyright (C) 2004 Ogochan.

This module is part of OSEKI.

	OSEKI is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
OSEKI, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with OSEKI so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_OSEKI_STORAGE_H
#define	_OSEKI_STORAGE_H
#include	"apistruct.h"

extern	ObjectType	NewObject(OsekiSession *state, int mode);
extern	Bool		OpenObject(OsekiSession *state, ObjectType obj, int mode);
extern	Bool		CloseObject(OsekiSession *state, ObjectType obj);
extern	Bool		DestroyObject(OsekiSession *state, ObjectType obj);
extern	int			WriteObject(OsekiSession *state, ObjectType obj,
								byte *buff, size_t size);
extern	int			ReadObject(OsekiSession *state, ObjectType obj,
							   byte *buff, size_t size);
extern	Bool	OsekiTransactionStart(OsekiSession *state);
extern	Bool	OsekiTransactionCommit(OsekiSession *state);
extern	Bool	OsekiTransactionAbort(OsekiSession *state);


#endif
