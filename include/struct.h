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

typedef	struct {
	size_t	n;
	struct {
		char	window[SIZE_NAME];
	}	close[15];
}	CloseWindows;


typedef	struct _ProcessNode	{
	char		term[SIZE_TERM+1]
	,			user[SIZE_USER+1];
	char		*window
	,			*widget
	,			*event
	,			*pstatus;
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
	Bool	(*ExecuteProcess)(MessageHandler *handler, ProcessNode *);
	int		(*StartBatch)(MessageHandler *handler, char *name, char *param);
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
#define	INIT_READYDC	0x02
#define	INIT_READYDB	0x04
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

#endif
