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

#ifndef	_SERVER_H
#define	_SERVER_H
#include	"port.h"

#define	FETURE_NULL		(byte)0x0000
#define	FETURE_CORE		(byte)0x0001
#define	FETURE_I18N		(byte)0x0002
#define	FETURE_EXPAND	(byte)0x0004
#define	FETURE_BLOB		(byte)0x0008
#define	FETURE_NETWORK	(byte)0x0010

#define	fFetureCore		(((TermFeture & FETURE_CORE) != 0) ? TRUE : FALSE)
#define	fFetureBlob		(((TermFeture & FETURE_BLOB) != 0) ? TRUE : FALSE)
#define	fFetureExpand	(((TermFeture & FETURE_EXPAND) != 0) ? TRUE : FALSE)
#define	fFetureI18N		(((TermFeture & FETURE_I18N) != 0) ? TRUE : FALSE)
#define	fFetureNetwork	(((TermFeture & FETURE_NETWORK) != 0) ? TRUE : FALSE)

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	char	*PortNumber;
GLOBAL	int		Back;
#ifdef	USE_SSL
GLOBAL	Bool	fSsl;
GLOBAL	Bool	fVerify;
GLOBAL	char	*KeyFile;
GLOBAL	char	*CertFile;
GLOBAL	char	*CA_Path;
GLOBAL	char	*CA_File;
#endif
GLOBAL	URL		Auth;
GLOBAL	byte	TermFeture;

extern	void	InitSystem(int argc, char **argv);
extern	void	ExecuteServer(void);

#endif
