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
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/wait.h>

#include	"directory.h"
#include	"dbgroup.h"
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
static	Bool	fVerbose;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"environment file name"							},
	{	"allsync",	BOOLEAN,	TRUE,	(void*)&fAllsync,
		"All Database sync"								},
	{	"check",	BOOLEAN,	TRUE,	(void*)&fTablecheck,
		"Table compare check only(no sync)"				},
	{	"v",		BOOLEAN,	TRUE,	(void*)&fVerbose,
		"Verbose mode"									},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	Bool	FirstNG = TRUE;

static	void
SetDefault(void)
{
	fVerbose = FALSE;
	D_Dir = NULL;
	Directory = "./directory";
}

static void
separator(void)
{
	printf(" --------------------------------------------------------------------------\n");	
}

static void
ms_print(char *name, char *master, char *slave)
{
	printf("| %-6s   | %-20s       |     | %-20s       |\n", name, master, slave);
}

static void
msr_print(char *result, char *master, int m_count, char *slave, int s_count)
{
	printf("| %-6s   | %-17.17s %8d | ==> | %-17.17s %8d |\n",result, master,m_count,  slave, s_count);
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
	if (FirstNG && !fVerbose) {
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

static	void
all_allsync(
	DBG_Struct	*master_dbg,
	DBG_Struct	*slave_dbg)
{
	Bool ret;
	
	ret = dbexist(master_dbg);
	if (!ret) {
		Error("ERROR: database \"%s\" does not exist.", GetDB_DBname(master_dbg,DB_UPDATE));
	}
	ret = dbexist(slave_dbg);	
	if (ret) {
		dropdb(slave_dbg);
		if (fVerbose) {
			printf("Drop database\n");
		}
	}
	ret = createdb(slave_dbg);
	if (!ret) {
		Error("ERROR: create database \"%s\" failed.", GetDB_DBname(slave_dbg,DB_UPDATE));		
	}
	if (fVerbose) {
		printf("Create database\n");
	}
	if (fVerbose) {
		printf("Database sync start\n");
	}
	ret = all_sync(master_dbg, slave_dbg);
	if (!ret) {
		Error("ERROR: database sync failed.");
	}
	printf("Success all sync\n");
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
		table->count = 0;
		table->ngkind = ' ';
	}
	ng_list->tables[ng_list->count] = table;
	ng_list->count++;
}

static TableList *
table_check(
	DBG_Struct	*master_dbg,
	DBG_Struct	*slave_dbg)
{
	int i, m, s, cmp, rcmp;
	TableList *master_list, *slave_list, *ng_list;

	master_list	= get_table_info(master_dbg, 'c');
	slave_list = get_table_info(slave_dbg, 'c');
	ng_list = NewTableList(master_list->count + slave_list->count);
	m = s = 0;
	for ( i=0; (master_list->count > m) && (slave_list->count > s); i++) {
		cmp = strcmp( master_list->tables[m]->name, slave_list->tables[s]->name);
		rcmp = (master_list->tables[m]->relkind - slave_list->tables[s]->relkind)* 2 + cmp;
		if ( rcmp == 0 ) {
			if (master_list->tables[m]->count == slave_list->tables[s]->count) {
				if (fVerbose) {
					msr_print("OK",
						   master_list->tables[m]->name,
						   master_list->tables[m]->count,
						   slave_list->tables[s]->name,
						   slave_list->tables[s]->count);
				}
			} else {
				pre_print(master_dbg, slave_dbg);
				msr_print("NG",
					   master_list->tables[m]->name,
					   master_list->tables[m]->count,
					   slave_list->tables[s]->name,
					   slave_list->tables[s]->count);
				add_ng_list(ng_list, slave_list->tables[s], 'c');
			}
			m++;
			s++;
		} else if ( rcmp < 0 ) {
			pre_print(master_dbg, slave_dbg);			
			msr_print("NG",
				   master_list->tables[m]->name,
				   master_list->tables[m]->count,
				   "",
				   0);
			add_ng_list(ng_list, master_list->tables[m], 's');			
			m++;
		} else if ( rcmp > 0 ) {
			pre_print(master_dbg, slave_dbg);			
			msr_print("NG",			
				   "",
				   0,
				   slave_list->tables[s]->name,
				   slave_list->tables[s]->count);
			add_ng_list(ng_list, slave_list->tables[s], 'd');			
			s++;
		}
	}
	add_ng_list(ng_list, NULL, ' ');	
	if (fVerbose){
		separator();
	}
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
	DBG_Struct	*rdbg;
	if (dbg->redirect != NULL){
		MASTERDB = StrDup(dbg->name);
		rdbg = dbg->redirect;
		SLAVEDB = StrDup(rdbg->name);
	}
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	DBG_Struct	*master_dbg, *slave_dbg;
	TableList *ng_list;
	
	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("dbsync",NULL);
	
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,FALSE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}

	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)lookup_master_slave,NULL);
	master_dbg = g_hash_table_lookup(ThisEnv->DBG_Table, MASTERDB);
	slave_dbg = g_hash_table_lookup(ThisEnv->DBG_Table, SLAVEDB);

	if (!master_dbg || !slave_dbg){
		Error("Illegal dbgroup.");		
	}

	if (fVerbose){
		info_print(master_dbg, slave_dbg);
	}

	if (!dbtype_check(master_dbg) || !dbtype_check(slave_dbg) ){
		Error("Sorry, does not support Database type.");
	}
	if (!dbexist(master_dbg)){
		Error("ERROR: database \"%s\" does not exist.", GetDB_DBname(master_dbg,DB_UPDATE));		
	}

	if (!dbexist(slave_dbg)) {
		fAllsync = TRUE;
	}

	if (!fAllsync) {	
		ng_list = table_check(master_dbg, slave_dbg);
		if (ng_list->tables[0]->name != NULL) {
			if (fTablecheck){
				printf("NG, synchronization of the database\n");
				exit(2);
			}
			ng_list_sync(master_dbg, slave_dbg, ng_list);			
		} else {
			if (fVerbose){
				printf("OK, synchronization of the database\n");			
			}
		}
	}
	
	if (fAllsync) {
		all_allsync(master_dbg, slave_dbg);
	}
	
	return	(0);
}
