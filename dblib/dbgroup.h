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

#ifndef	_DBGROUP_H
#define	_DBGROUP_H

#include	"libmondai.h"
#include	"net.h"
#include	"dblib.h"

extern	DB_Func	*NewDB_Func(void);
extern	DB_Func	*EnterDB_Function(char *name, DB_OPS *ops, DB_Primitives *primitive,
								  char *commentStart, char *commentEnd);
extern	void	OpenRedirectDB(DBG_Struct *dbg);
extern	void	ExecRedirectDBOP(DBG_Struct *dbg, char *sql);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	RecordStruct	**ThisDB;

GLOBAL	char	*DB_Host;
GLOBAL	char	*DB_Port;
GLOBAL	char	*DB_Name;
GLOBAL	char	*DB_User;
GLOBAL	char	*DB_Pass;

GLOBAL	Bool	fNoCheck;
GLOBAL	Bool	fNoRedirect;

#endif
