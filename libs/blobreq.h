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

#ifndef	_INC_BLOBREQ_H
#define	_INC_BLOBREQ_H

#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"
#include	"blobcom.h"

extern	MonObjectType	RequestNewBLOB(NETFILE *fp, PacketClass flag, int mode);
extern	Bool	RequestOpenBLOB(NETFILE *fp, PacketClass flag, int mode, MonObjectType obj);
extern	Bool	RequestCloseBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj);
extern	size_t	RequestWriteBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj,
								 byte *buff, size_t size);
extern	size_t	RequestReadBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj,
								byte *buff, size_t size);
extern	Bool	RequestExportBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj, char *fname);
extern	MonObjectType	RequestImportBLOB(NETFILE *fp, PacketClass flag, char *fname);
extern	Bool	RequestSaveBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj, char *fname);
extern	Bool	RequestCheckBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj);
extern	Bool	RequestDestroyBLOB(NETFILE *fp, PacketClass flag, MonObjectType obj);
extern	Bool	RequestStartBLOB(NETFILE *fp, PacketClass flag);
extern	Bool	RequestCommitBLOB(NETFILE *fp, PacketClass flag);
extern	Bool	RequestAbortBLOB(NETFILE *fp, PacketClass flag);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
