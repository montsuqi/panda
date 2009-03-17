/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_POSTGRES
#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<libpq-fe.h>
#include	<libpq/libpq-fs.h>
#include	"dbgroup.h"
#include	"PostgreSQLlib.h"
#include	"PostgreSQLutils.h"
#include	"debug.h"
		
const char *PG_DUMP = "pg_dump";
const char *PSQL = "psql";

extern PGconn	*
pg_connect(
	DBG_Struct	*dbg)
{
	PGconn *conn = NULL;
	
	if (dbg->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT) {
		/* database is already connected. */
		conn = PGCONN(dbg, DB_UPDATE);
	} else {
		if ((conn = PgConnect(dbg, DB_UPDATE)) != NULL){
			dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
		}
	}
	return conn;
}

extern void
pg_disconnect(
	DBG_Struct	*dbg)
{
	if (dbg->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT) {
		PQfinish(PGCONN(dbg, DB_UPDATE));
		dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_DISCONNECT;
	}
}

extern PGconn	*
template1_connect(
	DBG_Struct	*dbg)
{
	LargeByteString *conninfo;
	PGconn	*conn;

	conninfo = Template1Conninfo(dbg,DB_UPDATE);
	conn = PQconnectdb(LBS_Body(conninfo));
	FreeLBS(conninfo);
	
	return conn;
}

extern size_t
pg_escape(
	char *to,
	const char *from,
	size_t length)
{
	return PQescapeString (to, from, length);
}

extern PGresult	*
db_exec(
	PGconn	*conn,
	char *query)
{
	PGresult	*res;
	
	res = PQexec(conn, query);

	return res;
}

extern Bool
db_command(
	PGconn	*conn,
	char *query)
{
	Bool ret;
	PGresult	*res;
	
	res = PQexec(conn, query);
	if ( 	(res != NULL )
		 && (PQresultStatus(res) == PGRES_COMMAND_OK ) ) {
		ret = TRUE;
	} else {
		fprintf(stderr, "%s\n", PQerrorMessage(conn));		
		ret = FALSE;		
	}
	PQclear(res);
	return ret;
}


char **
make_pgopts(
	const char *command,
	DBG_Struct	*dbg)
{
	int pgoptc;
	char **pgoptv;

	char *option;

	pgoptv = malloc((255)*sizeof(char *));

	pgoptc = 0;	
	pgoptv[pgoptc++] = strdup(command);

	if ((option = GetDB_Host(dbg, DB_UPDATE)) != NULL){
		pgoptv[pgoptc++] = "-h";
		pgoptv[pgoptc++] = option;
	}
	if ((option = GetDB_PortName(dbg, DB_UPDATE)) != NULL){
		pgoptv[pgoptc++] = "-p";
		pgoptv[pgoptc++] = option;
	}
	if ((option = GetDB_User(dbg, DB_UPDATE)) != NULL){
		pgoptv[pgoptc++] = "-U";
		pgoptv[pgoptc++] = option;
	}

	pgoptv[pgoptc] = NULL;
	
	return pgoptv;
}

int optsize(char **optv)
{
	int i;
	
	for (i=0; optv[i] != NULL; i++ ){};
	return i;
}

static Bool
sync_wait(pid_t pg_dump_pid, pid_t restore_pid)
{
	pid_t	pid;
	int status;
	Bool ret = FALSE;
	
	for (;;)
	{
		pid = wait(&status);
		if ( pg_dump_pid ==  pid ) {
			if ( WIFEXITED(status) && (WEXITSTATUS(status) != 0) ) {
				fprintf(stderr, "Dump error(%d)\n", pid );				
				break;				
			}
		} else if (restore_pid ==  pid ) {
			if ( WIFEXITED(status) && (WEXITSTATUS(status) != 0) ) {
				fprintf(stderr, "Restore error(%d)\n", pid);
				break;
			} else {
				fprintf(stderr, "succcess restore\n");
				ret = TRUE;
			}
		} else if (pid == -1) {
			break;
		}
	}
	return ret;
}

static void
db_dump(int fd, char *pass, char **argv)
{
	char command[SIZE_BUFF];
	
	snprintf(command, SIZE_BUFF, "%s/%s", POSTGRES_BINDIR, PG_DUMP);
	setenv("PGPASSWORD", pass, 1);

	execv(command, argv);

	fprintf( stderr, "load program pg_dump\n" );
	perror("");
	exit(1);
}

static void
db_restore(int fd, char *pass, char **argv)
{
	char command[SIZE_BUFF];
	
	snprintf(command, SIZE_BUFF, "%s/%s", POSTGRES_BINDIR, PSQL);

	setenv("PGPASSWORD", pass, 1);
	execv(command, argv);	

	fprintf( stderr, "load program psql\n" );
	perror("");
	exit(1);
}

extern 	Bool
dbexist(DBG_Struct	*dbg)
{
	PGconn	*conn;
	PGresult	*res;
	Bool ret = FALSE;
	char sql[SIZE_BUFF];	

	conn = template1_connect(dbg);
	snprintf(sql, SIZE_BUFF, "SELECT datname FROM pg_database WHERE datname = '%s';\n", GetDB_DBname(dbg,DB_UPDATE));
	res = db_exec(conn, sql);
	if (PQntuples(res) == 1) {
		ret = TRUE;
	}
	PQclear(res);
	PQfinish(conn);
	
	return ret;
}

extern 	Bool
dropdb(DBG_Struct	*dbg)
{
	PGconn	*conn;
	Bool ret = FALSE;
	char sql[SIZE_BUFF];

	conn = template1_connect(dbg);
	snprintf(sql, SIZE_BUFF, "DROP DATABASE %s;\n", GetDB_DBname(dbg,DB_UPDATE));
	ret = db_command(conn, sql);
	
	PQfinish(conn);
	
	return ret;
}
extern 	Bool
createdb(DBG_Struct	*dbg)
{
	PGconn	*conn;
	Bool ret = FALSE;
	char sql[SIZE_BUFF];

	conn = template1_connect(dbg);
	snprintf(sql, SIZE_BUFF, "CREATE DATABASE %s;\n", GetDB_DBname(dbg,DB_UPDATE));
	ret = db_command(conn, sql);

	PQfinish(conn);
	return ret;
}

extern Bool
all_sync(DBG_Struct	*master_dbg, DBG_Struct *slave_dbg)
{
	int p1[2], moptc, soptc;
	char *master_pass, *slave_pass;
	char **master_argv, **slave_argv;
	pid_t	pg_dump_pid, restore_pid;
	Bool ret = FALSE;

	master_argv = make_pgopts(PG_DUMP, master_dbg);
	moptc = optsize(master_argv);
	master_argv[moptc++] = GetDB_DBname(master_dbg,DB_UPDATE);
	master_argv[moptc] = NULL;
	
	slave_argv = make_pgopts(PSQL, slave_dbg);
	soptc = optsize(slave_argv);
	slave_argv[soptc++] = "-v";
	slave_argv[soptc++] = "ON_ERROR_STOP=1";
	slave_argv[soptc++] = GetDB_DBname(slave_dbg,DB_UPDATE);
	slave_argv[soptc] = NULL;	
	
	master_pass = GetDB_Pass(master_dbg, DB_UPDATE);
	slave_pass = GetDB_Pass(slave_dbg, DB_UPDATE);	
	
	if ( pipe(p1) == -1 ) {
		fprintf( stderr, "cannot open pipe\n");
		return FALSE;
	}

	if ( (restore_pid = fork()) == 0 ){
		/* |psql */		
		close( p1[1] );
		close( 0 );
		dup2(p1[0], 0); /* (STDIN -> fd) */
		db_restore(p1[0], slave_pass, slave_argv);
	} else {
		if ( (pg_dump_pid = fork()) == 0 ) {
			/* pg_dump| */
			close( p1[0] );
			close( 1 );
			dup2( p1[1], 1); /* (STDOUT -> fd) */
			db_dump(p1[1], master_pass, master_argv);
		} else {
			/* parent */
			close( p1[0] );
			close( p1[1] );			
			ret = sync_wait(pg_dump_pid, restore_pid);
		}
	}
	return ret;
}

static char *
tableType( table_TYPE types )
{
	char		*type;
	type = (char *)malloc(1024);

	switch(types){
		case ALL:
			strcpy(type, "'r','v','i',''");
			break;
		case TABLE:
			strcpy(type, "'r'");
			break;
		case INDEX:
			strcpy(type, "'i'");
			break;
		case VIEW:
			strcpy(type, "'v'");
			break;
		default:
			break;
	}
	return type;
}

TableList	*
NewTableList(int count)
{
	TableList	*table_list;
	table_list = (TableList *) malloc(sizeof(TableList));
	table_list->count = 0;
	table_list->tables = NULL;
	table_list->tables = (Table **)malloc(count * sizeof(Table *));
	return table_list;
}

Table *
NewTable(void)
{
	Table		*table;
	table = (Table *)malloc(sizeof(Table));
	table->name = NULL;
	table->relkind = ' ';
	table->count = 0;	
	return table;
}

char *
queryTableList( table_TYPE types )
{
	char		*sql;
	char		*type;

	type = tableType(types);
	sql = (char *)malloc(1024);
	
	snprintf(sql, 1024,
			 " SELECT c.relname,  c.relkind"
			 "   FROM pg_catalog.pg_class AS c "
			 "   JOIN pg_catalog.pg_roles r ON r.oid = c.relowner "
			 "   LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
			 "  WHERE c.relkind IN (%s) "
			 "    AND n.nspname NOT IN ('pg_catalog', 'pg_toast')"
			 "    AND pg_catalog.pg_table_is_visible(c.oid) "
			 "  ORDER BY c.relkind, c.relname ;", type );
	free(type);
	
	return (sql);
}

void TableCount(
	PGconn	*conn,
	TableList *table_list )
{
	int i;
	char buff[1024];
	PGresult	*res;
	
	for (i = 0; i < table_list->count; i++) {
		if (table_list->tables[i]->relkind == 'i') continue;
		sprintf(buff, "SELECT count(*) FROM %s;\n", table_list->tables[i]->name);
		res = PQexec(conn, buff);
		table_list->tables[i]->count = atoi(PQgetvalue(res, 0,0));
		PQclear(res);
	}
}


TableList *
getTableList( PGconn	*conn )
{
	PGresult	*res;
	ExecStatusType status;
	TableList	*table_list;
	Table		*table;
	char		*sql;
	int			ntuples;
	int			i;

	sql = queryTableList(ALL);
	res = PQexec(conn, sql);
	free(sql);
	
	status = PQresultStatus(res);
	ntuples = PQntuples(res);
	table_list = NewTableList(ntuples);
	
	for (i = 0; i < ntuples; i++) {
		table = NewTable();
		table->name = strdup(PQgetvalue(res, i, 0));
		table->relkind = *(PQgetvalue(res, i, 1));
		table_list->tables[i] = table;
		table_list->count++;
	}
	PQclear(res);

	return table_list;
}

extern TableList *
get_table_info(
	DBG_Struct	*dbg,
	char opt)
{
	PGconn	*conn;
	TableList *tablelist;
	
	conn = pg_connect(dbg);
	tablelist = getTableList(conn);
	
	if (opt == 'c'){
		TableCount(conn, tablelist);
	}

	return tablelist;
}

#endif /* #ifdef HAVE_POSTGRES */
