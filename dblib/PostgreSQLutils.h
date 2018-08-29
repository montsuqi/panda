/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 Ogochan & JMA (Japan Medical Association).
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

#include "struct.h"
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

typedef enum table_TYPE { ALL, TABLE, INDEX, VIEW } table_TYPE;

typedef struct table {
  char *name;
  char relkind;
  char *count;
  char ngkind;
} Table;

typedef struct tablelist {
  int count;
  Table **tables;
} TableList;

typedef struct dbinfo {
  char *tablespace;
  char *template;
  char *encoding;
  char *lc_collate;
  char *lc_ctype;
} DBInfo;

typedef struct authinfo {
  char *pass;
  char *sslcert;
  char *sslkey;
  char *sslrootcert;
  char *sslcrl;
} AuthInfo;

extern Bool template1_check(DBG_Struct *dbg);
extern DBInfo *getDBInfo(DBG_Struct *dbg, char *dbname);
extern Bool dropdb(DBG_Struct *dbg);
extern Bool createdb(DBG_Struct *dbg, char *tablespace, char *template,
                     char *encoding, char *lc_collate, char *lc_ctype);
extern Bool delete_table(DBG_Struct *dbg, char *table_name);
extern Bool dbexist(DBG_Struct *dbg);
extern Bool dbactivity(DBG_Struct *dbg);
extern Bool all_sync(DBG_Struct *master_dbg, DBG_Struct *slave_dbg,
                     char *dump_opt, Bool verbose);
extern Bool table_sync(DBG_Struct *master_dbg, DBG_Struct *slave_dbg,
                       char *table_name);
extern TableList *get_table_info(DBG_Struct *dbg, char opt);
extern void printTableList(TableList *table_list);
extern void puttable(DBG_Struct *dbg);
extern TableList *NewTableList(int count);
extern Table *NewTable(void);

extern Bool pg_trans_begin(DBG_Struct *dbg);
extern Bool pg_trans_end(DBG_Struct *dbg);
extern Bool pg_trans_commit(DBG_Struct *dbg);
extern int redirector_check(DBG_Struct *dbg);

extern PGconn *pg_connect(DBG_Struct *dbg);
extern void pg_disconnect(DBG_Struct *dbg);
extern PGresult *db_exec(PGconn *conn, char *query);
extern Bool db_command(PGconn *conn, char *query);
