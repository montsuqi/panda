/*	OSEKI -- Object Store Engine Kernel Infrastructure

Copyright (C) 2004 Ogochan.

This module is part of OSEKI.

	OSEKI is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
OSEKI, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with OSEKI so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_OSEKI_PAGE_H
#define	_OSEKI_PAGE_H
#include	<stdint.h>
#include	"apistruct.h"

extern	OsekiSpace		*InitOseki(char *space);
extern	void		FinishOseki(OsekiSpace *blob);
extern	OsekiSession	*ConnectOseki(OsekiSpace *blob);
extern	void		DisConnectOseki(OsekiSession *state);

extern	pageno_t	NewPage(OsekiSession *state);
extern	void		*GetPage(OsekiSession *state, pageno_t page);
extern	void		*UpdatePage(OsekiSession *state, pageno_t page);
extern	void	ReleasePage(OsekiSession *state, pageno_t page,Bool fCommit);
extern	pageno_t	GetFreePage(OsekiSession *state);
extern	void		GetZeroPage(OsekiSession *state);
extern	void		UpdateZeroPage(OsekiSession *state);

extern	void	ReturnPage(OsekiSession *state, pageno_t no);
extern	void	CommitPages(OsekiSession *state);
extern	void	AbortPages(OsekiSession *state);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
