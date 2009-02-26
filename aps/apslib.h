/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan.
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

#define	MCP_PUT_NULL		"NULL"
#define	MCP_PUT_CURRENT		"CURRENT"
#define	MCP_PUT_NEW			"NEW"
#define	MCP_PUT_CLOSE		"CLOSE"
#define	MCP_PUT_CHANGE		"CHANGE"
#define	MCP_PUT_JOIN		"JOIN"
#define	MCP_PUT_FORK		"FORK"
#define	MCP_PUT_END			"EXIT"
#define	MCP_PUT_EXIT		MCP_PUT_END
#define	MCP_PUT_BACK		"BACK"
#define	MCP_PUT_CALL		"CALL"
#define	MCP_PUT_RETURN		"RETURN"

extern	int			MCP_PutWindow(ProcessNode *node, char *wname, char *type, char *widget);
extern	int			MCP_PutData(ProcessNode *node, char *rname);
extern	int			MCP_ReturnComponent(ProcessNode *node, char *event);
extern	RecordStruct	*MCP_GetWindowRecord(ProcessNode *node, char *name);
extern	int			MCP_ExecFunction(ProcessNode *node, char *rname,
									 char *pname, char *func, ValueStruct *data);
extern	ValueStruct	*MCP_GetDB_Define(char *name);

extern	void		*MCP_GetEventHandler(GHashTable *StatusTable, ProcessNode *node);
extern	void		MCP_RegistHandler(GHashTable *StatusTable, char *status,
									 char *event,void (*handler)(ProcessNode *));

#endif
