/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
*/
#define	MAIN
#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<crypt.h>

#include	"types.h"

#include	"dirs.h"
#include	"option.h"
#include	"misc.h"
#include	"dirs.h"
#include	"auth.h"
#include	"debug.h"

static	int		Uid;
static	int		Gid;
static	char	*Other;
static	char	*Pass;

static	ARG_TABLE	option[] = {
	{	"file",		STRING,		TRUE,	(void*)&PasswordFile,
		"パスワードファイル名"							},
	{	"u",		INTEGER,	FALSE,	(void*)&Uid,
		"user id"										},
	{	"g",		INTEGER,	FALSE,	(void*)&Gid,
		"group id"										},
	{	"o",		STRING,		FALSE,	(void*)&Other,
		"other options"									},
	{	"p",		STRING,		FALSE,	(void*)&Pass,
		"password"										},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PasswordFile = "./passwd";
	Uid = 0;
	Gid = 0;
	Pass = "";
	Other = "";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	PassWord	*pw;
	char		*p;

	SetDefault();
	fl = GetOption(option,argc,argv);
	
	AuthLoadPasswd(PasswordFile);
	if		(  !stricmp(g_basename(argv[0]),"gluseradd")  ) {
		if		(  Uid  ==  0  ) {
			Uid = AuthMaxUID() + 1;
		}
		AuthAddUser(fl->name,crypt(Pass,AuthMakeSalt()),Gid,Uid,Other);
	} else
	if		(  !stricmp(g_basename(argv[0]),"gluserdel")  ) {
		AuthDelUser(fl->name);
	} else
	if		(  !stricmp(g_basename(argv[0]),"glusermod")  ) {
		if		(  ( pw = AuthGetUser(fl->name) )  !=  NULL  ) {
			if		(  Uid  ==  0  ) {
				Uid = pw->uid;
			}
			if		(  Gid  ==  0  ) {
				Gid = pw->gid;
			}
			if		(  *Other  ==  0  ) {
				Other = pw->other;
			}
			if		(  *Pass  ==  0  ) {
				p = pw->pass;
			} else {
				p = crypt(Pass,AuthMakeSalt());
			}
			AuthAddUser(fl->name,p,Gid,Uid,Other);
		}
	} else
	if		(  !stricmp(g_basename(argv[0]),"glauth")  ) {
		if		(  AuthAuthUser(fl->name,Pass)  !=  NULL  ) {
			printf("OK\n");
		} else {
			printf("NG\n");
		}
	}
	AuthSavePasswd(PasswordFile);
	return	(0);
}
