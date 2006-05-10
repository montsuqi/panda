/*
 * PANDA -- a simple transaction monitor
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

#ifndef	_INC_WFC_H
#define	_INC_WFC_H
#include	"queue.h"
#include	"struct.h"
#include	"net.h"
#include	"blob.h"

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

extern	void	ChangeLD(SessionData *data, LD_Node *newld);
extern	LargeByteString	*GetScreenData(SessionData *data, char *name);

GLOBAL	char		**BindTable;
GLOBAL	int			cBind;
GLOBAL	GHashTable	*ComponentHash;
GLOBAL	GHashTable	*APS_Hash;
GLOBAL	GHashTable	*MQ_Hash;
GLOBAL	Bool		fShutdown;
GLOBAL	int			MaxRetry;
GLOBAL	BLOB_State	*BlobState;
GLOBAL	Bool		fLoopBack;

#endif
