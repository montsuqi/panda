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

#include	<net.h>
#include	<libmondai.h>

#define	SIZE_FUNC		16
typedef	struct {
	char	func[SIZE_FUNC];
	int		rc;
	int		blocks;
	int		rno;
	int		pno;
}	DBCOMM_CTRL;

typedef	struct _DBG_Struct	{
	char		*name;					/*	group name		*/
	char		*type;					/*	DBMS type name	*/
	struct	_DB_Func		*func;
	NETFILE		*fpLog;
	LargeByteString	*redirectData;
	char		*file;
	Port		*redirectPort;
	struct	_DBG_Struct	*redirect;
	GHashTable	*dbt;
	int			priority;
	/*	DB depend	*/
	Port		*port;
	char		*dbname;
	char		*user;
	char		*pass;
	/*	DB connection variable	*/
	Bool		fConnect;
	void		*conn;
}	DBG_Struct;

typedef	void	(*DB_FUNC)(DBCOMM_CTRL *, RecordStruct *);
typedef	void	(*DB_EXEC)(DBG_Struct *, char *);
typedef	Bool	(*DB_FUNC_NAME)(char *, DBCOMM_CTRL *, RecordStruct *);

typedef	struct _DB_Func	{
	DB_FUNC_NAME	access;
	DB_EXEC			exec;
	GHashTable		*table;
	char			*commentStart
	,				*commentEnd;
}	DB_Func;

typedef	struct {
	char	*name;
	DB_FUNC	func;
}	DB_OPS;

extern	void	InitDB_Process(void);
extern	void	ExecDBG_Operation(DBG_Struct *dbg, char *name);
extern	void	ExecDBOP(DBG_Struct *dbg, char *sql);
extern	void	ExecDB_Process(DBCOMM_CTRL *ctrl, RecordStruct *rec);
extern	int		ExecDB_Function(char *name, char *tname, RecordStruct *rec);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	GHashTable	*DB_Table;

#endif
