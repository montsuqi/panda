/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

#include	"marshaller.h"

#ifndef	_INC_INTERFACE_H
#define	_INC_INTERFACE_H

extern	int			UI_Version();

extern	void		UI_list_config();
extern	void		UI_load_config(char *configname);

extern	void		UI_ShowWindow(char *sname, PacketClass type);
extern	void		UI_UpdateScreen(char *window);
extern	void		UI_ResetScrolledWindow(char *window);
extern	void		UI_GrabFocus(char *window, char *widgetName);
extern  void 		UI_GetWidgetData(WidgetData *data);
extern	WidgetType	UI_GetWidgetType(char *windowname, char *widgetName);
extern	void		UI_ResetTimer(char *window);
extern	gboolean	UI_IsWidgetName(char *name);
extern	gboolean	UI_IsWidgetName2(char *dataname);
extern	void		UI_ErrorDialog(const char *msg);
extern	int			UI_AskPass(char *buff, size_t buflen, const char *prmpt);

extern	gboolean	UI_BootDialogRun(void);

extern	void		UI_Init(int argc, char **argv);
extern	void		UI_InitStyle(void);
extern	void		UI_Main(void);
extern  void		UI_Final(void);

#endif
