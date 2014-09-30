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

#ifndef	_INC_MONSYS_H
#define	_INC_MONSYS_H
#include	"dbgroup.h"

#define BATCH_TABLE "monbatch"
#define BATCH_LOG_TABLE "monbatch_log"
#define BATCH_TIMEOUT 86400

extern	DBG_Struct *GetDBG_monsys(void);
extern	void		CheckBatchExist(DBG_Struct	*dbg,int pgid);
extern	void		CheckBatchPg(void);
extern	char	   *Coding_monsys(DBG_Struct *dbg,const char *src);
extern	char       *Escape_monsys(DBG_Struct *dbg, const char *src);

#endif
