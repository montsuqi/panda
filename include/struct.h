/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#define	DBOP_CLOSE		5

typedef	struct {
	char			*name;
	LargeByteString	*proc;
	ValueStruct		*args;
}	DB_Operation;

typedef	struct {
	char			*name;
	GHashTable		*opHash;
	int				ocount;
	DB_Operation	**ops;
	ValueStruct		*args;
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

#define NOCONNECT   0x00
#define CONNECT     0x01
#define UNCONNECT   0x02
#define FAILURE     0x03
#define DISCONNECT  0x04
#define REDFAILURE  0x05

typedef	struct _DBG_Struct	{
	int			id;
	char		*name;					/*	group name				*/
	char		*type;					/*	DBMS type name			*/
	struct	_DB_Func		*func;
	GHashTable	*dbt;
	int			priority;				/*	commit priority			*/
	char		*coding;				/*	DB backend coding		*/
	/*	DB depend	*/
	Port		*port;
	char		*dbname;
	char		*user;
	char		*pass;
	/*	DB connection variable	*/
	int			fConnect;
	void		*conn;
	int			dbstatus;
	/*	DB redirect variable	*/
	Port		*redirectPort;
	struct	_DBG_Struct	*redirect;
	NETFILE		*fpLog;
	LargeByteString	*redirectData;
	LargeByteString	*checkData;
	char		*file;
}	DBG_Struct;

typedef	void	(*DB_FUNC)(DBG_Struct *, DBCOMM_CTRL *, RecordStruct *, ValueStruct *);

typedef struct	{
	int		(*exec)(DBG_Struct *, char *, Bool);
	Bool	(*access)(DBG_Struct *, char *, DBCOMM_CTRL *, RecordStruct *, ValueStruct *);
}	DB_Primitives;

typedef	struct _DB_Func	{
	DB_Primitives	*primitive;
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
		byte	PutType;
		char	window[SIZE_NAME];
	}	control[15];
}	WindowControl;

typedef	struct _ProcessNode	{
	char		term[SIZE_TERM+1]
	,			user[SIZE_USER+1];
	char		window[SIZE_NAME+1]
	,			widget[SIZE_NAME+1]
	,			event[SIZE_EVENT]
	,			pstatus
	,			dbstatus;
	RecordStruct	*mcprec;
	RecordStruct	*linkrec;
	RecordStruct	*sparec;
	RecordStruct	**scrrec;
	RecordStruct	*thisscrrec;
	size_t		cWindow;
	GHashTable	*bhash;
	size_t		cBind;
	size_t		textsize;
	WindowControl	w;
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
	void	(*ReadyExecute)(MessageHandler *handler, char *loadpath);
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
	RecordStruct	*sparec;
	RecordStruct	**windows;
	size_t			cWindow;
	GHashTable		*whash;
	WindowBind		**binds;
	size_t			cBind;
	GHashTable		*bhash;
	char		*loadpath
	,			*handlerpath;
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
	char		*home;
	size_t		arraysize
	,			textsize;
	size_t		cDB;
	GHashTable	*DB_Table;
	GHashTable	*BatchTable;
	RecordStruct	**db;
	char		*loadpath
	,			*handlerpath;
}	BD_Struct;

typedef	struct {
	char		*dir;
	Port		*port;
	URL			*auth;
}	BLOB_Struct;

typedef	struct {
	char		*name;
	char		*BaseDir
	,			*D_Dir
	,			*RecordDir;
	Port		*WfcApsPort
	,			*TermPort
	,			*ControlPort;
	size_t		cLD
	,			cBD
	,			cDBD
	,			linksize
	,			stacksize;
	BLOB_Struct		*blob;
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
	char		*ApsPath
		,		*WfcPath
		,		*RedPath
		,		*DbPath;
}	DI_Struct;

#endif
