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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_CRYPT_H
#include	<crypt.h>
#endif
#include	<sys/types.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<time.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"auth.h"
#include	"message.h"
#include	"debug.h"

#undef	Error
#define	Error(msg)	printf("%s\n",(msg))

static	GHashTable		*PasswdTable;
static	PassWord		**PasswdPool;
static	int				cPass;

extern	const	char	*
AuthMakeSalt(void)
{
	struct	timeval	tv;
	static	char	result[40];

	result[0] = '\0';
	strcpy(result, "$1$");  /* magic for the new MD5 crypt() */

	/*
	 * Generate 8 chars of salt, the old crypt() will use only first 2.
	 */
	gettimeofday(&tv, (struct timezone *) 0);
	strcat(result, l64a(tv.tv_usec));
	strcat(result, l64a(tv.tv_sec + getpid() + clock()));

	if (strlen(result) > 3 + 8)  /* magic+salt */
		result[11] = '\0';

	return result;
}

extern	void
AuthClearEntry(void)
{
	int		i;

	if		(  PasswdTable  !=  NULL  ) {
		g_hash_table_destroy(PasswdTable);
		for	( i = 0 ; PasswdPool[i] != NULL ; i ++ ) {
			xfree(PasswdPool[i]);
		}
		xfree(PasswdPool);
	}
	PasswdTable = NewNameHash();
	cPass = 0;
	PasswdPool = (PassWord **)xmalloc(sizeof(PassWord *));
	PasswdPool[0] = NULL;
}

extern	void
AuthAddEntry(
	PassWord	*pw)
{
	PassWord	**pp;

	g_hash_table_insert(PasswdTable,pw->name,pw);
	pp = (PassWord **)xmalloc(sizeof(PassWord *) * ( cPass + 2 ));
	if		(  cPass  >  0  ) {
		memcpy(pp,PasswdPool,sizeof(PassWord *) * cPass);
	}
	xfree(PasswdPool);
	PasswdPool = pp;
	PasswdPool[cPass] = pw;
	PasswdPool[cPass+1] = NULL;
	cPass ++;
}

extern	void
AuthAddUser(
	char	*name,
	char	*pass,
	int		gid,
	int		uid,
	char	*other)
{
	PassWord	*pw;
	Bool	fNew;

dbgmsg(">AuthAddUser");
	if		(  ( pw = g_hash_table_lookup(PasswdTable,name) )  ==  NULL  ) {
		pw = New(PassWord);
		fNew = TRUE;
	} else {
		fNew = FALSE;
	}
	strncpy(pw->name,name,SIZE_USER);
	strcpy(pw->pass,pass);
	pw->gid = gid;
	pw->uid = uid;
	strncpy(pw->other,other,SIZE_OTHER);
	if		(  fNew  ) {
		AuthAddEntry(pw);
	}
dbgmsg("<AuthAddUser");
}

extern	void
AuthDelUser(
	char	*name)
{
	PassWord	*pw;

	if		(  ( pw = g_hash_table_lookup(PasswdTable,name) )  !=  NULL  ) {
		g_hash_table_remove(PasswdTable,name);
		*pw->name = 0;
	}
}

extern	PassWord	*
AuthAuthUser(
	char	*name,
	char	*pass)
{
	PassWord	*pw;

	if		(  ( pw = g_hash_table_lookup(PasswdTable,name) )  !=  NULL  ) {
		if		(  strcmp(pw->pass, crypt(pass,pw->pass))  !=  0  ) {
			pw = NULL;
		}
	}
	return	(pw);
}

extern	PassWord	*
AuthGetUser(
	char	*name)
{
	return	(g_hash_table_lookup(PasswdTable,name));
}

extern	void
AuthLoadPasswd(
	char	*fname)
{
	char	buff[SIZE_BUFF];
	FILE	*fp;
	PassWord	*pw;
	char	*p
	,		*q;

	AuthClearEntry();
	if		(  ( fp = fopen(fname,"r") )  ==  NULL  ) {
		Error("can not open password file");
		return;
	}
	while	(  fgets(buff, SIZE_BUFF, fp)  !=  NULL  ) {
		pw = New(PassWord);
		p = buff;
		q = strchr(p,':');	*q = 0;
		strcpy(pw->name,p);
		p = q + 1;	q = strchr(p,':');	*q = 0;
		strcpy(pw->pass,p);
		p = q + 1;	q = strchr(p,':');	*q = 0;
		pw->uid = atoi(p);
		p = q + 1;	q = strchr(p,':');	*q = 0;
		pw->gid = atoi(p);
		p = q + 1;	q = strchr(p,'\n');	*q = 0;
		strcpy(pw->other,p);
		AuthAddEntry(pw);
	}
	fclose(fp);
}

extern	Bool
AuthSingle(
	char	*fname,
	char	*name,
	char	*pass,
	char	*other)
{
	char	buff[SIZE_BUFF];
	FILE	*fp;
	char	*p
	,		*q;
	Bool	rc;

	if		(  ( fp = fopen(fname,"r") )  ==  NULL  ) {
		Error("can not open password file");
		rc = FALSE;
	} else {
		rc = FALSE;
		while	(  fgets(buff, SIZE_BUFF, fp)  !=  NULL  ) {
			p = buff;
			q = strchr(p,':');	*q = 0;
			if		(  strcmp(p,name)  ==  0  ) {
				p = q + 1;	q = strchr(p,':');	*q = 0;
				if		(  strcmp(p, crypt(pass,p))  ==  0  ) {
					p = q + 1;	q = strchr(p,':');	*q = 0;
					p = q + 1;	q = strchr(p,':');	*q = 0;
					p = q + 1;	q = strchr(p,'\n');	*q = 0;
					if		(  other  !=  NULL  ) {
						strcpy(other,p);
					}
					rc = TRUE;
				}
				break;
			}
		}
		fclose(fp);
	}
	return	(rc);
}

extern	void
AuthSavePasswd(
	char	*fname)
{
	int		i;
	PassWord	*pw;
	FILE	*fp;

	if		(  ( fp = fopen(fname,"w") )  ==  NULL  ) {
		Error("can not open password file");
		return;
	}
	for	( i = 0 ; PasswdPool[i]  !=  NULL ; i ++ ) {
		pw = PasswdPool[i];
		if		(  *pw->name  !=  0  ) {
			fprintf(fp,"%s:%s:%d:%d:%s\n",
					pw->name,pw->pass,pw->uid,pw->gid,pw->other);
		}
	}
	fclose(fp);
}

static	int		MaxUID;
static	void
CheckMax(
	char	*name,
	PassWord	*pwd,
	void	*data)
{
	if		(  pwd->uid  >  MaxUID  ) {
		MaxUID = pwd->uid;
	}
}

extern	int
AuthMaxUID(void)
{
	MaxUID = 0;

	g_hash_table_foreach(PasswdTable,(GHFunc)CheckMax,NULL);
	return	(MaxUID);
}
