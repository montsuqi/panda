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
#ifndef	_HANDLER_H
#define	_HANDLER_H

#include	"struct.h"
#include	"apslib.h"
#include	"dblib.h"

extern	void	InitiateHandler(void);
extern	void	InitiateBatchHandler(void);
extern	void	ReadyDC(void);
extern	void	ReadyHandlerDB(MessageHandler *handler);
extern	void	ReadyOnlineDB(void);
extern	void	ExecuteProcess(ProcessNode *node);
extern	void	StopDC(void);
extern	void	StopHandlerDB(MessageHandler *handler);
extern	void	StopOnlineDB(void);
extern	void	CleanUpHandlerDB(MessageHandler *handler);
extern	void	CleanUpOnlineDB(void);
extern	void	CleanUpOnlineDC(void);

extern	int		StartBatch(char *name, char *para);

extern	void	MakeCTRL(DBCOMM_CTRL *ctrl, ValueStruct *mcp);
extern	void	MakeMCP(ValueStruct *mcp, DBCOMM_CTRL *ctrl);
extern	void	DumpDB_Node(DBCOMM_CTRL *ctrl);

#undef	GLOBAL
#ifdef	_HANDLER
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	char		*Directory;
GLOBAL	LD_Struct	*ThisLD;
GLOBAL	BD_Struct	*ThisBD;
GLOBAL	DBD_Struct	*ThisDBD;
GLOBAL	char		*LibPath;

GLOBAL	size_t		TextSize;
GLOBAL	int			nCache;

#undef	GLOBAL
#endif

