/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_PROTOCOL_H
#define	_INC_PROTOCOL_H
#include	"queue.h"

typedef	Bool	(*RecvHandler)(GtkWidget *, NETFILE *);
typedef	Bool	(*SendHandler)(char *, GtkWidget *, NETFILE *);

typedef	struct {
	GtkType		type;
	RecvHandler	rfunc;
	SendHandler	sfunc;
}	HandlerNode;

extern	void		InitProtocol(void);
extern	void		CheckScreens(NETFILE *fp, Bool);
extern	XML_Node	*ShowWindow(char *wname, byte type);
extern	Bool		SendConnect(NETFILE *fpComm, char *apl);
extern	Bool		RecvWidgetData(GtkWidget *widget, NETFILE *fp);
extern	void		RecvValue(NETFILE *fp, char *longname);
extern	Bool		GetScreenData(NETFILE *fpComm);
extern  void		SendWindowData(void);
extern	void		SendEvent(NETFILE *fpComm, char *window, char *widget, char *event);
extern	void		AddClass(GtkType type, RecvHandler rfunc, SendHandler sfunc);
extern	void		ResetTimer(GtkWindow *window);


#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	Bool	Protocol1;
GLOBAL	Bool	Protocol2;

#endif
