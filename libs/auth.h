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

#ifndef	_INC_AUTH_H
#define	_INC_AUTH_H

#include	<glib.h>
/*
 *	user:pass:gid:uid:others
 */

#include	"const.h"
#ifndef	SIZE_USER
#define	SIZE_USER		64
#endif
#ifndef	SIZE_PASS
#define	SIZE_PASS		64
#endif
#ifndef	SIZE_OTHER
#define	SIZE_OTHER		256
#endif

typedef	struct {
	char	name[SIZE_USER+1];
	char	pass[SIZE_PASS+1];
	int		gid;
	int		uid;
	char	other[SIZE_OTHER+1];
}	PassWord;

extern	void	AuthClearEntry(void);
extern	const	char	*AuthMakeSalt(void);
extern	void	AuthAddEntry(PassWord *pw);
extern	void	AuthAddUser(char *name, char *pass, int gid, int uid, char *other);
extern	void	AuthDelUser(char *name);
extern	PassWord	*AuthAuthUser(char *name, char *pass);
extern	PassWord	*AuthGetUser(char *name);
extern	void	AuthLoadPasswd(char *fname);
extern	void	AuthSavePasswd(char *fname);
extern	int		AuthMaxUID(void);
extern	Bool	AuthSingle(char *fname, char *name, char *pass, char *other);

#endif
