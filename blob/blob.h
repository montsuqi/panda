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

#define	BLOB_VERSION	1

#define	SIZE_BLOB_HEADER	4
#define	BLOB_V1_HEADER		"PLO1"

#define	BLOB_OPEN_CREATE	0x01
#define	BLOB_OPEN_READ		0x02
#define	BLOB_OPEN_WRITE		0x04
#define	BLOB_OPEN_APPEND	0x08

#if	BLOB_VERSION == 1
#include	"blob_v1.h"
#endif

extern	int		VersionBLOB(char *space);

#endif
