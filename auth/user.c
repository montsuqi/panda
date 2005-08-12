/*
PANDA -- a simple transaction monitor
Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
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

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#ifdef HAVE_CRYPT_H
#include	<crypt.h>
#endif

#include	"types.h"

#include	"dirs.h"
#include	"option.h"
#include	"auth.h"
#include	"message.h"
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

	InitMessage(g_basename(argv[0]),NULL);
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
