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

#ifndef	_INC_APSAPL_H
#define	_INC_APSAPL_H

#include	"const.h"
#include	<libmondai.h>
#include	"enum.h"

#define	MCP_PUT_NULL		0
#define	MCP_PUT_CURRENT		1
#define	MCP_PUT_NEW			2
#define	MCP_PUT_CLOSE		3
#define	MCP_PUT_CHANGE		4
#define	MCP_PUT_BACK		5
#define	MCP_PUT_JOIN		6
#define	MCP_PUT_FORK		7
#define	MCP_PUT_EXIT		8

#ifndef	CloseWindows
typedef	struct {
	size_t	n;
	struct {
		char	window[SIZE_NAME];
	}	close[15];
}	_CloseWindows;
#define	CloseWindows	_CloseWindows
#endif

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

extern	int			MCP_PutWindow(ProcessNode *node, char *wname, int type);
extern	RecordStruct	*MCP_GetWindowRecord(ProcessNode *node, char *name);
extern	int			MCP_ExecFunction(ProcessNode *node, char *rname,
									 char *pname, char *func, ValueStruct *data);
extern	ValueStruct	*MCP_GetDB_Define(char *name);

extern	void		*MCP_GetEventHandler(GHashTable *StatusTable, ProcessNode *node);
extern	void		MCP_RegistHandler(GHashTable *StatusTable, char *status,
									 char *event,void (*handler)(ProcessNode *));

#endif
