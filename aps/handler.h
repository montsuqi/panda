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

#include	"BDparser.h"
#include	"DBparser.h"
#include	"LDparser.h"

#include	"apslib.h"
#include	"dblib.h"

typedef	struct {
	char	*name;
	Bool	fUse;
	Bool	(*ExecuteProcess)(ProcessNode *);
	int		(*StartBatch)(char *name, char *param);
	/*	DC function	*/
	void	(*ReadyDC)(void);
	void	(*StopDC)(void);
	void	(*CleanUpDC)(void);
	/*	DB function	*/
	void	(*ReadyDB)(void);
	void	(*StopDB)(void);
	void	(*CleanUpDB)(void);
}	MessageHandlerClass;

extern	void	InitiateHandler(void);
extern	void	InitiateBatchHandler(void);
extern	void	ReadyDC(void);
extern	void	ReadyDB(void);
extern	void	ExecuteProcess(ProcessNode *node);
extern	void	StopDC(void);
extern	void	CleanUp(void);
extern	void	StopDB(void);
extern	void	CleanUpDB(void);
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

