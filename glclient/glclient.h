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

#ifndef	_INC_GLCLIENT_H
#define	_INC_GLCLIENT_H

#include <glade/glade.h>
#include	"libmondai.h"
#include	"net.h"

#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

typedef	struct {
	char		*name;
	GladeXML	*xml;
	GtkWindow	*window;
	GHashTable	*UpdateWidget;
}	XML_Node;

extern	char	*CacheFileName(char *name);
extern	void	ExitSystem(void);

GLOBAL	GHashTable	*WindowTable;
GLOBAL	XML_Node	*FocusedScreen;

GLOBAL	char		*CurrentApplication;

GLOBAL	Bool	fInRecv;

GLOBAL	NETFILE	*fpComm;
GLOBAL	char	*User;
GLOBAL	char	*Pass;
GLOBAL	Bool	fMlog;
#ifdef	USE_SSL
GLOBAL	Bool	fSsl;
GLOBAL	Bool	fVerify;
GLOBAL	char	*KeyFile;
GLOBAL	char	*CertFile;
GLOBAL	char	*CA_Path;
GLOBAL	char	*CA_File;
#endif

#endif
