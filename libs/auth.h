/*
PANDA -- a simple transaction monitor
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
