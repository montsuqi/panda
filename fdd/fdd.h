/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2004 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_FDD_H
#define	_INC_FDD_H

#define	ERROR_FILE_NOT_FOUND	1
#define	ERROR_FDD_NOT_READY		2
#define	ERROR_INCOMPLETE		3
#define	ERROR_WRITE				4

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#ifdef	USE_SSL
GLOBAL	Bool	fSsl;
GLOBAL	Bool	fVerify;
GLOBAL	char	*KeyFile;
GLOBAL	char	*CertFile;
GLOBAL	char	*CA_Path;
GLOBAL	char	*CA_File;
#endif

#endif
