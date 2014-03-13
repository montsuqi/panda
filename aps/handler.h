/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_HANDLER_H
#define	_HANDLER_H

#include	"struct.h"
#include	"apslib.h"
#include	"dblib.h"

extern	void	InitiateHandler(void);
extern	void	InitiateBatchHandler(void);
extern	void	ReadyDC(void);
extern	void	ReadyHandlerDB(MessageHandler *handler);
extern	int		ReadyOnlineDB(char *appname);
extern	void	ExecuteProcess(ProcessNode *node);
extern	void	StopDC(void);
extern	void	StopHandlerDB(MessageHandler *handler);
extern	void	StopOnlineDB(void);
extern	void	CleanUpHandlerDB(MessageHandler *handler);
extern	void	CleanUpOnlineDB(void);
extern	void	CleanUpOnlineDC(void);

extern	int		StartBatch(char *name, char *para);

extern	void	ExpandStart(char *line, char *start, char *path,
							char *module, char *param);
extern	void	ReadyExecuteCommon(MessageHandler *handler);

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

#undef	GLOBAL
#endif

