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
#include	"wfcdata.h"
#include	"net.h"

#define	SCREEN_DATA_NULL		0
#define	SCREEN_DATA_CONNECT		1
#define	SCREEN_DATA_END			2

typedef	struct {
	char			window[SIZE_NAME+1];
	char			widget[SIZE_NAME+1];
	char			event[SIZE_NAME+1];
	char			cmd[SIZE_LONGNAME+1];
	char			user[SIZE_USER+1];
	char			term[SIZE_TERM+1];
	char			host[SIZE_HOST+1];
	char			agent[SIZE_TERM+1];
	int				status;
	unsigned char	puttype;
	unsigned char	feature;
	WindowStack		w;
	GHashTable		*Windows;
}	ScreenData;

typedef	struct {
	unsigned char	puttype;
	RecordStruct	*rec;
}	WindowData;

extern	NETFILE	*ConnectTermServer(char *url,ScreenData *scr);
extern	Bool	SendTermServer(NETFILE *fp, ScreenData *scr, ValueStruct *value);
extern	Bool	RecvTermServerHeader(NETFILE *fp,ScreenData *scr);
extern	void	RecvTermServerData(NETFILE *fp, ScreenData *scr);
extern	Bool	SendTermServerEnd(NETFILE *fp, ScreenData *scr);

#endif
