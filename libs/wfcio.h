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

#ifndef	_WFCIO_H
#define	_WFCIO_H
#include	"const.h"
#include	"enum.h"
#include	"driver.h"
#include	"wfcdata.h"
#include	"net.h"

extern	NETFILE	*ConnectTermServer(char *url,ScreenData *scr);
extern	Bool	SendTermServer(NETFILE *fp, ScreenData *scr, ValueStruct *value);
extern	Bool	RecvTermServerHeader(NETFILE *fp,ScreenData *scr);
extern	void	RecvTermServerData(NETFILE *fp, ScreenData *scr);
extern	Bool	SendTermServerEnd(NETFILE *fp, ScreenData *scr);

#endif
