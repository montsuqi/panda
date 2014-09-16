/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	<time.h>
#include	<errno.h>

#include	"libmondai.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"monsys.h"
#include	"gettext.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;
static	char	*DBG_Name;
static	char	*Command;
static	char	*Output;
static	Bool	Redirect = FALSE;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"dbg",		STRING,		TRUE,	(void*)&DBG_Name,
		N_("db group name")								},
	{	"r",		BOOLEAN,	TRUE,	(void*)&Redirect,
		N_("redirect query")						},
	{	"c",		STRING,		TRUE,	(void*)&Command,
		N_("run only single command")					},
	{	"o",		STRING,		TRUE,	(void*)&Output,
		N_("out put type [JSON] [XML] [CSV]...")		},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory = "./directory";
	Redirect = FALSE;
}

static	void
InitSystem(void)
{
	char *dir;
	InitMessage("monsql",NULL);
	if ((dir = getenv("MCP_DIRECTORY_PATH")) != NULL) {
		Directory = dir;
	}
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,P_NONE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
}

static void
OutPutValue(
	char *type,
	ValueStruct	*value)
{
	ConvFuncs	*conv;
	static CONVOPT *ConvOpt;
	char *buf;

	if (!value) {
		return;
	}
	conv = GetConvFunc(type);
	if ( conv == NULL) {
		Error("Output type error. [%s] does not support.", type);
	}
	ConvOpt = NewConvOpt();
	buf = xmalloc(conv->SizeValue(ConvOpt,value));
	conv->PackValue(ConvOpt,buf,value);
	printf("%s\n", buf);
	xfree(buf);
}

static void
SingleCommand(
	DBG_Struct	*dbg,
	Bool		redirect,
	char *sql)
{
	ValueStruct	*ret;


	OpenDB(dbg);
	TransactionStart(dbg);

	ret = ExecDBQuery(dbg, sql, redirect, DB_UPDATE);
	OutPutValue(Output, ret);
	FreeValueStruct(ret);

	TransactionEnd(dbg);
	CloseDB(dbg);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	DBG_Struct	*dbg;

	SetDefault();
	GetOption(option,argc,argv,NULL);
	InitSystem();

	if (DBG_Name != NULL) {
		dbg = GetDBG(DBG_Name);
	} else {
		dbg = GetDBG_monsys();
	}

	if (!dbg) {
		Error("DBG [%s] does not found.", DBG_Name);
	}

	dbg->dbt = 	NewNameHash();
	if ( Command ) {
		SingleCommand(dbg, Redirect, Command);
	}

	return 0;
}
