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
#include	<json.h>
#include	<libmondai.h>
#include	<RecParser.h>

#include	"enum.h"
#include	"const.h"
#include	"wfcdata.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"dbutils.h"
#include	"monsys.h"
#include	"monpushevent.h"
#include	"option.h"
#include	"gettext.h"
#include	"message.h"
#include	"debug.h"

static char *Directory;
static char *DBConfig;
static char *JSONFile;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"dbconfig",	STRING,		TRUE,	(void*)&DBConfig,
		"database connection config file" 				},
	{	"json",		STRING,		TRUE,	(void*)&JSONFile,
		"json input file" 								},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory	= "/usr/lib/jma-receipt/lddef/directory";
	DBConfig	= NULL;
	JSONFile	= NULL;
}

static	int
MonPushEvent(
	DBG_Struct *dbg,
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
	buf = g_malloc0(size+1);
	memcpy(buf,_buf,size);
	g_free(_buf);
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

	return push_event_via_value(dbg,val);
}

static	int
MonPushEventJSON(
	DBG_Struct *dbg)
{
	json_object *obj,*event,*body;
	gchar *buf,*_buf;
	gsize size;
	int ret;

	if (!g_file_get_contents(JSONFile,&_buf,&size,NULL)) {
		g_error("read json file failure:%s\n",JSONFile);
	}
	buf = g_malloc0(size+1);
	memcpy(buf,_buf,size);
	g_free(_buf);

	obj = json_tokener_parse(buf);
	g_free(buf);
	if (obj == NULL || is_error(obj)) {
		g_error("invalid json file:%s",JSONFile);
	}
	if (!json_object_object_get_ex(obj,"event",&event)) {
		g_error("invalid json: need \"event\" object");
	}
	if (!json_object_object_get_ex(obj,"body",&body)) {
		g_error("invalid json: need \"body\" object");
	}

	ret = push_event_via_json(dbg,(const char*)json_object_get_string(event),body);
	json_object_put(obj);
	return ret;
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
	int ret;

	setlocale(LC_CTYPE,"ja_JP.UTF-8");
	SetDefault();
	GetOption(option,argc,argv,NULL);
	InitSystem();

	dbg = GetDBG_monsys();
	dbg->dbt = NewNameHash();

	if (OpenDB(dbg) != MCP_OK ) {
		g_error("OpenDB failure");
	}
	monpushevent_setup(dbg);

	TransactionStart(dbg);
	if (JSONFile != NULL) {
		ret = MonPushEventJSON(dbg);
	} else {
		if (argc < 3) {
			g_print("%% monpushevent <recfile> <COBOL data file>\n");
			exit(1);
		}
		ret = MonPushEvent(dbg,argv[1],argv[2]);
	}
	TransactionEnd(dbg);
	CloseDB(dbg);

	if (ret) {
		return 0;
	} else {
		Error("monpushevent failure");
	}
}
