/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2004 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_ACTION_H
#define	_INC_ACTION_H
#include	"glclient.h"

extern	void		GrabFocus(GtkWidget *widget);
extern	void		ResetTimer(GtkWindow *window);
extern	XML_Node	*ShowWindow(char *wname, byte type);
extern	void		DestroyWindow(char *sname);
extern	void		DestroyWindowAll();
extern	void		ClearWindowTable(void);
extern	void		UpdateWidget(GtkWidget *widget, void *user_data);

#endif
