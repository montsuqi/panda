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

#ifndef	_INC_DRIVER_H
#define	_INC_DRIVER_H

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"glterm.h"

typedef	struct {
	char		window[SIZE_NAME+1];
	char		widget[SIZE_NAME+1];
	char		event[SIZE_EVENT+1];
	char		cmd[SIZE_BUFF];
	char		user[SIZE_USER+1];
	char		term[SIZE_TERM+1];
	char		other[SIZE_OTHER+1];
	int			status;
	GHashTable	*Windows;
}	ScreenData;

typedef	struct {
	Bool			fNew;
	byte			PutType;
	RecordStruct	*rec;
}	WindowData;

extern	WindowData		*SetWindowName(char *name);
extern	Bool			PutWindow(WindowData *win, byte type);
extern	RecordStruct	*GetWindowRecord(char *wname);
extern	void			LinkModule(char *name);
extern	ScreenData		*NewScreenData(void);

/*	for easy C programing	*/
extern	ValueStruct	*GetWindowValue(char *name);
extern	WindowData	*PutWindowByName(char *wname, byte type);

#define	ThisWindow	(ThisScreen->window)
#define	ThisWidget	(ThisScreen->widget)
#define	ThisEvent	(ThisScreen->event)
#define	ThisUser	(ThisScreen->user)
#define	ThisTerm	(ThisScreen->term)

#undef	GLOBAL
#ifdef	_DRIVER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
GLOBAL	ScreenData	*ThisScreen;	/*	current applications ScreenData		*/
#undef	GLOBAL
#endif
