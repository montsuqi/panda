/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 JMA (Japan Medical Association).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

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
#include	<stdarg.h>
#include	<unistd.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/wait.h>

#include	"directory.h"
#include	"dbgroup.h"
#include	"comm.h"
#include	"redirect.h"
#include	"dirs.h"
#include	"PostgreSQLutils.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

char *MASTERDB = "";
char *SLAVEDB  = "log";

static	char	*Directory;
static	Bool	fAllsync;
static	Bool	fTablecheck;
static	Bool	fOVerbose;
static	Bool	fVerbose;
static	Bool	fReverse;
static  char    *Master = NULL;
static  char    *Slave = NULL;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"environment file name"							},

	{	"master",		STRING,		TRUE,	(void*)&Master,
		"master dbg name"							},
	{	"slave",		STRING,		TRUE,	(void*)&Slave,
		"slave dbg name"							},

	{	"allsync",	BOOLEAN,	TRUE,	(void*)&fAllsync,
		"All Database sync"								},
	{	"reverse",	BOOLEAN,	TRUE,	(void*)&fReverse,
		"Reverse(slave->master) sync"						},
	{	"check",	BOOLEAN,	TRUE,	(void*)&fTablecheck,
		"Table compare check only(no sync)"				},
	{	"v",		BOOLEAN,	TRUE,	(void*)&fOVerbose,
		"Verbose mode"									},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	Bool	FirstNG = TRUE;

static void
verPrintf(
	char *format,
	...)
{
	va_list	va;
	if (fVerbose) {
		va_start(va, format);
		vprintf(format, va);
		va_end(va);
	}
}

static	void
SetDefault(void)
{
	fVerbose = FALSE;
	fReverse = FALSE;
	D_Dir = NULL;
	Directory = "./directory";
}

static void
separator(void)
{
	verPrintf(" --------------------------------------------------------------------------\n");
}

static void
ms_print(char *name, char *master, char *slave)
{
	verPrintf("| %-6s   | %-20s       |     | %-20s       |\n", name, (master ? master : ""), (slave ? slave : ""));
}

static void
msr_print(char *result, char *master, char *m_count, char *slave, char *s_count)
{
	verPrintf("| %-6s   | %-17.17s %8s | ==> | %-17.17s %8s |\n",result, master,m_count,  slave, s_count);
}

static void
info_print(DBG_Struct	*master_dbg, DBG_Struct *slave_dbg)
{
	separator();
	ms_print("", "Master", "Slave");
	separator();
	ms_print("type", master_dbg->type, slave_dbg->type);
	ms_print("host", GetDB_Host(master_dbg,DB_UPDATE), GetDB_Host(slave_dbg,DB_UPDATE));
	ms_print("name", GetDB_DBname(master_dbg,DB_UPDATE), GetDB_DBname(slave_dbg,DB_UPDATE));
	ms_print("user", GetDB_User(master_dbg,DB_UPDATE), GetDB_User(slave_dbg,DB_UPDATE));
	separator();
}

static void
pre_print(DBG_Struct	*master_dbg, DBG_Struct *slave_dbg)
{
	if ( FirstNG ) {
		info_print(master_dbg, slave_dbg);
		FirstNG = FALSE;
	}
}

static Bool
dbtype_check(DBG_Struct	*dbg)
{
	Bool rc = FALSE;
	if ( strncmp(dbg->type, "PostgreSQL", 10) == 0 ) {
		rc = TRUE;
	}
	return rc;
}

static	Bool
all_allsync(
	DBG_Struct	*master_dbg,
	DBG_Struct	*slave_dbg)
{
	Bool ret;
	DBInfo *dbinfo;
	char *tablespace = NULL;
	char *template = NULL;
	char *encoding = NULL;
	char *lc_collate = NULL;
	char *lc_ctype = NULL;
	char *dump_opt = NULL;
	int  i = 0;

	ret = dbexist(master_dbg);
	if (!ret) {
		Warning("ERROR: database \"%s\" does not exist.", GetDB_DBname(master_dbg,DB_UPDATE));
		return FALSE;
	}
	dbinfo = getDBInfo(master_dbg, GetDB_DBname(master_dbg,DB_UPDATE));
	if	( slave_dbg->coding != NULL ) {
		encoding = slave_dbg->coding;
	} else {
		encoding = dbinfo->encoding;
	}
	lc_collate = dbinfo->lc_collate;
	lc_ctype = dbinfo->lc_ctype;
	template = dbinfo->template;

	while(redirector_check(slave_dbg) != 0){
		sleep(1);
		i++;
		if (i>10){
			Warning("dbredirector is connected.");
			break;
		}
	}

	if ((!template1_check(slave_dbg)) || (dbactivity(slave_dbg) > 0)){
		dump_opt = StrDup("-c");
	} else {
		if ( dbexist(slave_dbg) ){
			verPrintf("Drop database\n");
			dropdb(slave_dbg);
		}
		verPrintf("Create database name=%s, template=%s, encoding=%s, lc_collate=%s, lc_ctype=%s \n", slave_dbg->name, template, encoding, lc_collate, lc_ctype);
		ret = createdb(slave_dbg, tablespace, template, encoding, lc_collate, lc_ctype);
		if (!ret) {
			Warning("ERROR: create database \"%s\" failed.", GetDB_DBname(slave_dbg,DB_UPDATE));
			return FALSE;
		}
	}
	verPrintf("Database sync start\n");
	ret = all_sync(master_dbg, slave_dbg, dump_opt, fVerbose);
	if (!ret) {
		Warning("ERROR: database sync failed.");
		return FALSE;
	}
	Message("Success all sync\n");
	return TRUE;
}

static void
add_ng_list(
	TableList *ng_list,
	Table *ng_table,
	char ngkind)
{
	Table *table;

	table = NewTable();
	if (ng_table != NULL){
		table->name = StrDup(ng_table->name);
		table->relkind = ng_table->relkind;
		table->count = ng_table->count;
		table->ngkind = ngkind;
	} else {
		table->name = NULL;
		table->relkind = ' ';
		table->count = StrDup("0");
		table->ngkind = ' ';
	}
	ng_list->tables[ng_list->count] = table;
	ng_list->count++;
}

static char*
table_name(
	TableList *table_list,
	int num)
{
	char *ret;
	if (table_list->count > num ){
		ret = table_list->tables[num]->name;
	} else {
		ret = "";
	}
	return ret;
}

static char
table_relkind(
	TableList *table_list,
	int num)
{
	char ret;
	if (table_list->count > num ){
		ret = table_list->tables[num]->relkind;
	} else {
		ret = 'z';
	}
	return ret;
}

static TableList *
table_check(
	DBG_Struct	*master_dbg,
	DBG_Struct	*slave_dbg)
{
	int i, m, s, cmp, rcmp;
	TableList *master_list, *slave_list, *ng_list;

	pg_trans_begin(master_dbg);
	pg_trans_begin(slave_dbg);
	master_list = get_table_info(master_dbg, 'c');
	slave_list = get_table_info(slave_dbg, 'c');
	pg_trans_commit(master_dbg);
	pg_trans_commit(slave_dbg);

	ng_list = NewTableList(master_list->count + slave_list->count);
	m = s = 0;
	for ( i=0; (master_list->count > m) || (slave_list->count > s); i++) {
		cmp = strcmp( table_name(master_list,m),table_name(slave_list,s) );
		cmp = ((cmp > 0)?(1):((cmp < 0)?(-1):cmp));
		rcmp = (table_relkind(master_list,m) - table_relkind(slave_list,s))*10 + cmp;
		if ( rcmp == 0 ) {
			if (!strcmp(master_list->tables[m]->count,slave_list->tables[s]->count)) {
				if (fVerbose){
					msr_print("OK",
						master_list->tables[m]->name,
						master_list->tables[m]->count,
						slave_list->tables[s]->name,
						slave_list->tables[s]->count);
				}
			} else {
				fVerbose = TRUE;
				pre_print(master_dbg, slave_dbg);
				msr_print("NG",
					   master_list->tables[m]->name,
					   master_list->tables[m]->count,
					   slave_list->tables[s]->name,
					   slave_list->tables[s]->count);
				fVerbose = fOVerbose;
				add_ng_list(ng_list, slave_list->tables[s], 'c');
			}
			m++;
			s++;
		} else if ( rcmp < 0 ) {
			fVerbose = TRUE;
			pre_print(master_dbg, slave_dbg);
			msr_print("NG",
				   master_list->tables[m]->name,
				   master_list->tables[m]->count,
				   "",
				   "0");
			fVerbose = fOVerbose;
			add_ng_list(ng_list, master_list->tables[m], 's');
			m++;
		} else if ( rcmp > 0 ) {
			fVerbose = TRUE;
			pre_print(master_dbg, slave_dbg);
			msr_print("NG",
				   "",
				   "0",
				   slave_list->tables[s]->name,
				   slave_list->tables[s]->count);
			fVerbose = fOVerbose;
			add_ng_list(ng_list, slave_list->tables[s], 'd');
			s++;
		}
	}
	add_ng_list(ng_list, NULL, ' ');
	if (!FirstNG){
		fVerbose = TRUE;
	}
	separator();
	fVerbose = fOVerbose;

	pg_disconnect(master_dbg);
	pg_disconnect(slave_dbg);

	return ng_list;
}

static Bool
ng_list_check(TableList *ng_list)
{
	Bool rc = TRUE;
	int i;

	for ( i=0; ng_list->tables[i]->name != NULL; i++) {
		if (ng_list->tables[i]->ngkind != 'c'){
			if (fVerbose) {
/*				printf("Sorry, the synchronization of the schema has not been supported yet.\n"); */
				printf("Allsync is executed.\n");
			}
			fAllsync = TRUE;
			rc = FALSE;
			break;
		}
	}
	return rc;
}

static void
ng_list_sync(
	DBG_Struct	*master_dbg,
	DBG_Struct	*slave_dbg,
	TableList *ng_list)
{
	int i;
	if (ng_list_check(ng_list)) {
		if (fVerbose) {
			printf("Sync start...\n");
		}
		for ( i=0; ng_list->tables[i]->name != NULL; i++) {
			if (ng_list->tables[i]->relkind == 'r') {
				if (fVerbose) {
					printf("Delete TABLE %s\n", ng_list->tables[i]->name);
				}
				delete_table(slave_dbg, ng_list->tables[i]->name);
				printf("Sync TABLE %s\n", ng_list->tables[i]->name);
				table_sync(master_dbg, slave_dbg, ng_list->tables[i]->name);
			}
		}
		if (fVerbose) {
			printf("Sync finish\n");
		}
	}
}

static	void
lookup_master_slave(
	char		*name,
	DBG_Struct	*dbg,
	void		*dummy)
{
	static	int  lookup_id = 0;

	if ( (dbg->redirect != NULL) && (dbg->id <= lookup_id)
		&& (dbg->redirectorMode == REDIRECTOR_MODE_PATCH) ) {
		lookup_id = dbg->id;
		if (!fReverse) {
			MASTERDB = StrDup(dbg->name);
			SLAVEDB = StrDup(dbg->redirect->name);
		} else {
			MASTERDB = StrDup(dbg->redirect->name);
			SLAVEDB = StrDup(dbg->name);
		}
	}
}

static	NETFILE	*
connect_dbredirector(
		DBG_Struct	*rdbg)
{
	int		fh;
	NETFILE	*fp = NULL;

	if		( ( rdbg->redirectPort  !=  NULL )
			&& (( fh = ConnectSocket(rdbg->redirectPort,SOCK_STREAM) )  >  0 ) ) {
		fp = SocketToNet(fh);
	}
	return fp;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	DBG_Struct	*master_dbg, *slave_dbg;
	TableList *ng_list;
	NETFILE	*fp;
	time_t start, end;
	int n, h, m, s;
	Bool ret = TRUE;

	SetDefault();
	GetOption(option,argc,argv,NULL);
	InitMessage("dbsync",NULL);

	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,FALSE);

	fVerbose = fOVerbose;

	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	if (Master == NULL ) {
		Master = StrDup("");
	}
	if (Slave != NULL)  {
		master_dbg = g_hash_table_lookup(ThisEnv->DBG_Table, Master);
		slave_dbg = g_hash_table_lookup(ThisEnv->DBG_Table, Slave);
	} else {
		g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)lookup_master_slave,NULL);
		master_dbg = g_hash_table_lookup(ThisEnv->DBG_Table, MASTERDB);
		slave_dbg = g_hash_table_lookup(ThisEnv->DBG_Table, SLAVEDB);
	}

	if (!master_dbg || !slave_dbg){
		Error("Illegal dbgroup.");
	}
	Message("Start [\"%s\"] -> [\"%s\"]", master_dbg->name, slave_dbg->name);
	if (!dbtype_check(master_dbg) || !dbtype_check(slave_dbg) ){
		Error("Sorry, does not support Database type.");
	}
	if (!template1_check(master_dbg)){
		Error("ERROR: database can not access server %s", GetDB_Host(master_dbg,DB_UPDATE));
	}
	if (!template1_check(slave_dbg)){
/*		Error("ERROR: database can not access server %s", GetDB_Host(slave_dbg,DB_UPDATE)); */
	}
	if (fVerbose){
		pre_print(master_dbg, slave_dbg);
	}
	if (!dbexist(master_dbg)){
		Error("ERROR: database \"%s\" does not exist.", GetDB_DBname(master_dbg,DB_UPDATE));
	}
	if ( !fTablecheck && !dbexist(slave_dbg)) {
		fAllsync = TRUE;
	}
	fp = connect_dbredirector(slave_dbg);
	if (!fAllsync) {
		if (fp){
			SendPacketClass(fp,RED_SYNC_WAIT);
			if (RecvPacketClass(fp) != RED_SYNC_WAIT){
				Error("unknown packet");
			}
		}

		ng_list = table_check(master_dbg, slave_dbg);
		if (ng_list->tables[0]->name != NULL) {
			if (fTablecheck){
				Message("NG, synchronization of the database");
				exit(2);
			}
#if 0
			ng_list_sync(master_dbg, slave_dbg, ng_list);
#else
			fAllsync = TRUE;
#endif
		} else {
			if (fVerbose){
				Message("OK, synchronization of the database");
			}
		}
	}

	if (fAllsync) {
		if (fp){
			SendPacketClass(fp,RED_SYNC_WAIT);
			if (RecvPacketClass(fp) != RED_SYNC_WAIT){
				Error("unknown packet");
			}
			SendPacketClass(fp,RED_SYNC_START);
		}
		Message("Synchronous begin.");
		time(&start);
		ret = all_allsync(master_dbg, slave_dbg);
		time(&end);
		n = (int)difftime(end, start);
		h = (n/60/60); m = (n/60)-(h*60) ;s = (n)-(m*60);
		MessageLogPrintf("Synchronous end. processing time %02d:%02d:%02d\n",
										 h,m,s);
		if (fp){
			SendPacketClass(fp,RED_SYNC_END);
			CloseNet(fp);
		}
	}
	if (!ret){
		return	2;
	}
	Message("Done.");
	return	0;
}
