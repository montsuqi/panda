/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<string.h>
#include    <sys/types.h>
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>
#include	<sys/file.h>
#include	<locale.h>
#include	<libmondai.h>
#include	<RecParser.h>

#include	"enum.h"
#include	"const.h"
#include	"wfcdata.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"dbutils.h"
#include	"monsys.h"
#include	"pushevent.h"
#include	"option.h"
#include	"gettext.h"
#include	"message.h"
#include	"debug.h"

static char *Directory;
static char *DBConfig;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"dbconfig",	STRING,		TRUE,	(void*)&DBConfig,
		"database connection config file" 				},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory	= "/usr/lib/jma-receipt/lddef/directory";
	DBConfig	= NULL;
}

static	void
MonPushEvent(
	const char *recfile,
	const char *datafile)
{
	gchar *_buf,*buf;
	gsize size;
	ValueStruct *val;
	CONVOPT *conv;

	RecParserInit();
	conv = NewConvOpt();
	ConvSetSize(conv,500,100);
	ConvSetCodeset(conv,"euc-jisx0213");

	if (!g_file_get_contents(recfile,&_buf,&size,NULL)) {
		g_error("read rec file failure:%s\n",recfile);
	}
	buf = g_realloc(_buf,size+1);
    buf[size] = 0;
	val = RecParseValueMem(buf,NULL);
	if (val == NULL) {
		g_error("Error: unable to read rec %s\n",recfile);
	}
	g_free(buf);

	if (!g_file_get_contents(datafile,&buf,&size,NULL)) {
		g_error("read cobol data file failure:%s\n",datafile);
	}
    OpenCOBOL_UnPackValue(conv,(unsigned char*)buf,val);
	g_free(buf);

	if (!PushEvent_via_ValueStruct(val)) {
		exit(1);
	}
}

static	void
InitSystem(void)
{
	char *dir;
	InitMessage("monpusheventp",NULL);
	if ( (dir = getenv("MON_DIRECTORY_PATH")) != NULL ) {
		Directory = dir;
	}
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,P_NONE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	SetDBConfig(DBConfig);
}

extern	int
main(
	int		argc,
	char	*argv[])
{
	DBG_Struct *dbg;

	setlocale(LC_CTYPE,"ja_JP.UTF-8");
	SetDefault();
	GetOption(option,argc,argv,NULL);
	InitSystem();

	if (argc < 3) {
		g_print("%% monpushevent <recfile> <COBOL data file>\n");
		exit(1);
	}

	dbg = GetDBG_monsys();
	dbg->dbt = NewNameHash();

	monpushevent_setup(dbg);

	TransactionStart(dbg);
	MonPushEvent(dbg,argv[1],argv[2]);
	TransactionEnd(dbg);
	CloseDB(dbg);

	return 0;
}
