/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_DBLIB_H
#define	_INC_DBLIB_H
#include	"struct.h"
#include	<net.h>
#include	<libmondai.h>

extern	void	InitDB_Process(void);
extern	void	ExecDBOP(DBG_Struct *dbg, char *sql);
extern	void	ExecDB_Process(DBCOMM_CTRL *ctrl, RecordStruct *rec, ValueStruct *args);
extern	void	TransactionStart(DBG_Struct *dbg);
extern	void	TransactionEnd(DBG_Struct *dbg);
extern	void	OpenDB(DBG_Struct *dbg);
extern	void	CloseDB(DBG_Struct *dbg);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	GHashTable	*DB_Table;

#endif
