/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2003 Ogochan.

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

/*
*	struct define
*/

#ifndef	_INC_STRUCT_H
#define	_INC_STRUCT_H
#include	<glib.h>
#include	"const.h"
#include	"libmondai.h"
#include	"port.h"
#include	"net.h"

typedef	struct {
	char	***item;
}	KeyStruct;

#define	DBOP_SELECT		0
#define	DBOP_FETCH		1
#define	DBOP_UPDATE		2
#define	DBOP_INSERT		3
#define	DBOP_DELETE		4

typedef	struct {
	char			*name;
	LargeByteString	*proc;
	ValueStruct		*args;
}	SQL_Operation;

typedef	struct {
	char			*name;
	GHashTable		*opHash;
	int				ocount;
	SQL_Operation	**ops;
}	PathStruct;


typedef	struct {
	KeyStruct	*pkey;
	PathStruct	**path;
	GHashTable	*paths;
	GHashTable	*use;
	int			pcount;
	void		*dbg;
}	DB_Struct;

#define	RECORD_NULL		0
#define	RECORD_DB		1

typedef	struct _RecordStruct	{
	char		*name;
	ValueStruct	*value;
	int		type;
	union {
		DB_Struct	*db;
	}	opt;
}	RecordStruct;

typedef	struct {
	char	func[SIZE_FUNC];
	int		rc;
	int		blocks;
	int		rno;
	int		pno;
}	DBCOMM_CTRL;

typedef	struct _DBG_Struct	{
	char		*name;					/*	group name			*/
	char		*type;					/*	DBMS type name		*/
	struct	_DB_Func		*func;
	NETFILE		*fpLog;
	LargeByteString	*redirectData;
	char		*file;
	Port		*redirectPort;
	struct	_DBG_Struct	*redirect;
	GHashTable	*dbt;
	int			priority;				/*	commit priority		*/
	char		*coding;				/*	DB backend coding	*/
	/*	DB depend	*/
	Port		*port;
	char		*dbname;
	char		*user;
	char		*pass;
	/*	DB connection variable	*/
	Bool		fConnect;
	void		*conn;
}	DBG_Struct;

typedef	void	(*DB_FUNC)(DBG_Struct *, DBCOMM_CTRL *, RecordStruct *);
typedef	void	(*DB_EXEC)(DBG_Struct *, char *);
typedef	Bool	(*DB_FUNC_NAME)(DBG_Struct *, char *, DBCOMM_CTRL *, RecordStruct *);

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

typedef	struct {
	size_t	n;
	struct {
		char	window[SIZE_NAME];
	}	close[15];
}	CloseWindows;

typedef	struct _ProcessNode	{
	char		term[SIZE_TERM+1]
	,			user[SIZE_USER+1];
	char		window[SIZE_NAME+1]
	,			widget[SIZE_NAME+1]
	,			event[SIZE_EVENT]
	,			pstatus;
	RecordStruct	*mcprec;
	RecordStruct	*linkrec;
	RecordStruct	*sparec;
	RecordStruct	**scrrec;
	size_t		cWindow;
	GHashTable	*whash;
	size_t		textsize;
	CloseWindows	w;
}	ProcessNode;

typedef	struct	{
	char	*name;
	byte	fInit;
	struct	_MessageHandlerClass	*klass;
	ConvFuncs			*serialize;
	CONVOPT				*conv;
	char				*start;
	char				*loadpath;
	void				*private;
}	MessageHandler;

typedef	struct _MessageHandlerClass	{
	char	*name;
	void	(*ReadyExecute)(MessageHandler *handler);
	Bool	(*ExecuteDC)(MessageHandler *handler, ProcessNode *);
	int		(*ExecuteBatch)(MessageHandler *handler, char *name, char *param);
	/*	DC function	*/
	void	(*ReadyDC)(MessageHandler *handler);
	void	(*StopDC)(MessageHandler *handler);
	void	(*CleanUpDC)(MessageHandler *handler);
	/*	DB function	*/
	void	(*ReadyDB)(MessageHandler *handler);
	void	(*StopDB)(MessageHandler *handler);
	void	(*CleanUpDB)(MessageHandler *handler);
}	MessageHandlerClass;

#define	INIT_LOAD		0x01
#define	INIT_EXECUTE	0x02
#define	INIT_READYDC	0x04
#define	INIT_READYDB	0x08
#define	INIT_STOPDC		0x10
#define	INIT_STOPDB		0x20
#define	INIT_CLEANDB	0x40
#define	INIT_CLEANDC	0x80

typedef	struct {
	char			*name;
	MessageHandler	*handler;
	char			*module;
	int				ix;
	RecordStruct	*rec;
}	WindowBind;

typedef	struct {
	char		*name;
	char		*group;
	char		*home;
	size_t		arraysize
	,			textsize;
	int			nCache;
	size_t		cDB;
	GHashTable	*DB_Table;
	RecordStruct	**db;
	size_t		nports;
	Port		**ports;
	Port		*wfc;
	RecordStruct	*sparec;
	size_t		cWindow;
	WindowBind	**window;
	GHashTable	*whash;
}	LD_Struct;

typedef	struct {
	char		*name;
	size_t		arraysize
	,			textsize;
	size_t		cDB;
	GHashTable	*DBD_Table;
	RecordStruct	**db;
}	DBD_Struct;

typedef	struct {
	char			*module;
	MessageHandler	*handler;
}	BatchBind;

typedef	struct {
	char		*name;
	size_t		arraysize
	,			textsize;
	size_t		cDB;
	GHashTable	*DB_Table;
	GHashTable	*BatchTable;
	RecordStruct	**db;
}	BD_Struct;

typedef	struct {
	char		*name;
	char		*BaseDir
	,			*D_Dir
	,			*RecordDir;
	Port		*WfcApsPort;
	size_t		cLD
	,			cBD
	,			cDBD
	,			linksize
	,			stacksize;
	RecordStruct	*mcprec;
	RecordStruct	*linkrec;
	GHashTable	*LD_Table;
	GHashTable	*BD_Table;
	GHashTable	*DBD_Table;
	LD_Struct	**ld;
	BD_Struct	**bd;
	DBD_Struct	**db;
	int			mlevel;
	int			cDBG;
	DBG_Struct	**DBG;
	GHashTable	*DBG_Table;
}	DI_Struct;

#endif
