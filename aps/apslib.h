/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2006 Ogochan.
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

#ifndef	_INC_APSAPL_H
#define	_INC_APSAPL_H

#include	"const.h"
#include	<libmondai.h>
#include	"struct.h"
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

extern	int			MCP_PutWindow(ProcessNode *node, char *wname, int type);
extern	RecordStruct	*MCP_GetWindowRecord(ProcessNode *node, char *name);
extern	int			MCP_ExecFunction(ProcessNode *node, char *rname,
									 char *pname, char *func, ValueStruct *data);
extern	ValueStruct	*MCP_GetDB_Define(char *name);

extern	void		*MCP_GetEventHandler(GHashTable *StatusTable, ProcessNode *node);
extern	void		MCP_RegistHandler(GHashTable *StatusTable, char *status,
									 char *event,void (*handler)(ProcessNode *));

#endif
