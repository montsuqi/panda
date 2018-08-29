/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

/*
*	struct define
*/

#ifndef _INC_STRUCT_H
#define _INC_STRUCT_H
#include <glib.h>
#include "const.h"
#include "libmondai.h"
#include "port.h"
#include "net.h"

typedef struct { char ***item; } KeyStruct;

#define SIZE_DBOP 6
#define DBOP_SELECT 0
#define DBOP_FETCH 1
#define DBOP_UPDATE 2
#define DBOP_INSERT 3
#define DBOP_DELETE 4
#define DBOP_CLOSE 5

#define DB_NONE 0x00
#define DB_READONLY 0x01
#define DB_UPDATE 0x02
#define DB_UNDEF 0x03
#define DB_FAIL 0x04

#define IsUsageNotFail(usage) (((usage)&DB_FAIL) == 0)
#define IsUsageUpdate(usage) (((usage)&DB_UPDATE) != 0)

typedef struct {
  char *name;
  LargeByteString *proc;
  ValueStruct *args;
} DB_Operation;

typedef struct {
  char *name;
  GHashTable *opHash;
  int ocount;
  int usage;
  DB_Operation **ops;
  ValueStruct *args;
} PathStruct;

typedef struct {
  KeyStruct *pkey;
  PathStruct **path;
  GHashTable *paths;
  GHashTable *use;
  int pcount;
  GHashTable *opHash;
  int ocount;
  DB_Operation **ops;
  struct _DBG_Struct *dbg;
  char *gname;
} DB_Struct;

#define RECORD_NULL 0
#define RECORD_DB 1

typedef struct _RecordStruct {
  char *name;
  ValueStruct *value;
  int type;
  union {
    DB_Struct *db;
  } opt;
} RecordStruct;

#define RecordDB(rec) ((rec)->opt.db)

typedef struct {
  char func[SIZE_FUNC];
  char rname[SIZE_NAME + 1];
  char pname[SIZE_NAME + 1];
  char user[SIZE_USER + 1];
  char term[SIZE_TERM + 1];
  int rc;
  int blocks;
  int rno;
  int pno;
  int rcount;
  int limit;
  int redirect;
  int usage;
  int fDBOperation;
  long time;
  RecordStruct *rec;
  ValueStruct *value;
  LargeByteString *src;
} DBCOMM_CTRL;

typedef struct {
  Port *port;
  char *dbname;
  char *user;
  char *pass;
  char *sslmode;
  char *sslcert;
  char *sslkey;
  char *sslrootcert;
  char *sslcrl;
} DB_Server;

#define DB_STATUS_NOCONNECT 0x00
#define DB_STATUS_CONNECT 0x01
#define DB_STATUS_UNCONNECT 0x02
#define DB_STATUS_FAILURE 0x03
#define DB_STATUS_DISCONNECT 0x04
#define DB_STATUS_REDFAILURE 0x05
#define DB_STATUS_LOCKEDRED 0x06
#define DB_STATUS_SYNC 0x07

typedef struct {
} DB_Process;

#define PROCESS_UPDATE 0
#define PROCESS_READONLY 1

#define REDIRECTOR_MODE_PATCH 0
#define REDIRECTOR_MODE_LOG 1

typedef struct _DBG_Struct {
  int id;
  char *name; /*	group name				*/
  char *type; /*	DBMS type name			*/
  struct _DB_Func *func;
  GHashTable *dbt; /*	DBs in this DBG, if this
                                     value is NULL, this DBG has no DB	*/
  int priority;    /*	commit priority			*/
  char *coding;    /*	DB backend coding		*/
  /*	DB redirect variable	*/
  Port *redirectPort;
  char *redirectName;
  struct _DBG_Struct *redirect;
  int redirectorMode;
  int auditlog;
  char *logTableName;
  NETFILE *fpLog;
  LargeByteString *redirectData;
  LargeByteString *checkData;
  uint64_t ticket_id;
  int count;
  int errcount;
  char *transaction_id;
  LargeByteString *last_query;
  char *file;
  int sumcheck;
  char *appname;
  DB_Server *server;
  void *conn;
  int dbstatus;
} DBG_Struct;

typedef ValueStruct *(*DB_FUNC)(DBG_Struct *, DBCOMM_CTRL *, RecordStruct *,
                                ValueStruct *);

typedef struct {
  int (*exec)(DBG_Struct *, char *, Bool);
  ValueStruct *(*access)(DBG_Struct *, DBCOMM_CTRL *, RecordStruct *,
                         ValueStruct *);
  ValueStruct *(*query)(DBG_Struct *, char *, Bool);
  Bool (*record)(DBG_Struct *, char *, RecordStruct *);
} DB_Primitives;

#define DB_PARSER_NULL 0
#define DB_PARSER_SQL 1
#define DB_PARSER_SCRIPT 2

typedef struct _DB_Func {
  int type;
  DB_Primitives *primitive;
  GHashTable *table;
  char *commentStart, *commentEnd;
} DB_Func;

typedef struct {
  char *name;
  DB_FUNC func;
} DB_OPS;

#define WINDOW_STACK_SIZE 16

typedef struct {
  size_t sp;
  struct {
    unsigned char puttype;
    char window[SIZE_NAME];
  } s[WINDOW_STACK_SIZE];
} WindowStack;

#define MESSAGE_TYPE_TERM 0
#define MESSAGE_TYPE_API 1
#define MESSAGE_TYPE_CHECK 2

typedef struct _ProcessNode {
  char uuid[SIZE_TERM + 1], user[SIZE_USER + 1];
  char window[SIZE_NAME + 1], widget[SIZE_NAME + 1], event[SIZE_NAME + 1],
      command, dbstatus;
  unsigned char puttype;
  RecordStruct *mcprec;
  RecordStruct *linkrec;
  RecordStruct *sparec;
  RecordStruct **scrrec;
  RecordStruct *thisscrrec;
  size_t cWindow;
  GHashTable *bhash;
  size_t cBind;
  size_t textsize;
  int messageType;
  WindowStack w;
} ProcessNode;

typedef struct {
  char *name;
  unsigned char fInit;
  struct _MessageHandlerClass *klass;
  ConvFuncs *serialize;
  CONVOPT *conv;
  char *start;
  char *loadpath;
  void *private;
} MessageHandler;

typedef struct _MessageHandlerClass {
  char *name;
  void (*ReadyExecute)(MessageHandler *handler, char *loadpath);
  Bool (*ExecuteDC)(MessageHandler *handler, ProcessNode *);
  int (*ExecuteBatch)(MessageHandler *handler, char *name, char *param);
  /*	DC function	*/
  void (*ReadyDC)(MessageHandler *handler);
  void (*StopDC)(MessageHandler *handler);
  void (*CleanUpDC)(MessageHandler *handler);
  /*	DB function	*/
  void (*ReadyDB)(MessageHandler *handler);
  void (*StopDB)(MessageHandler *handler);
  void (*CleanUpDB)(MessageHandler *handler);
} MessageHandlerClass;

#define INIT_LOAD 0x01
#define INIT_EXECUTE 0x02
#define INIT_READYDC 0x04
#define INIT_READYDB 0x08
#define INIT_STOPDC 0x10
#define INIT_STOPDB 0x20
#define INIT_CLEANDB 0x40
#define INIT_CLEANDC 0x80

typedef struct {
  char *name;
  MessageHandler *handler;
  char *module;
  RecordStruct *rec;
  Bool fAPI;
} WindowBind;

typedef struct {
  char *name;
  char *group;
  char *home;
  size_t arraysize, textsize;
  int nCache;
  size_t cDB;
  GHashTable *DB_Table;
  RecordStruct **db;
  size_t nports;
  Port **ports;
  RecordStruct *sparec;
  RecordStruct **windows;
  size_t cWindow;
  GHashTable *whash;
  WindowBind **binds;
  size_t cBind;
  GHashTable *bhash;
  char *loadpath, *handlerpath;
} LD_Struct;

typedef struct {
  char *name;
  size_t arraysize, textsize;
  size_t cDB;
  GHashTable *DBD_Table;
  RecordStruct **db;
} DBD_Struct;

typedef struct {
  char *module;
  MessageHandler *handler;
} BatchBind;

typedef struct {
  char *name;
  char *home;
  size_t arraysize, textsize;
  size_t cDB;
  GHashTable *DB_Table;
  GHashTable *BatchTable;
  RecordStruct **db;
  char *loadpath, *handlerpath;
} BD_Struct;

typedef struct {
  char *dir;
  Port *port;
} SysData_Struct;

typedef struct {
  char *name;
  char *BaseDir, *D_Dir, *RecordDir;
  Port *WfcApsPort, *TermPort, *ControlPort, *DBMasterPort;
  char *DBMasterAuth;
  size_t cLD, cBD, cDBD, stacksize;
  SysData_Struct *sysdata;
  RecordStruct *auditrec;
  RecordStruct *mcprec;
  RecordStruct *linkrec;
  char *InitialLD;
  GHashTable *LD_Table;
  GHashTable *BD_Table;
  GHashTable *DBD_Table;
  LD_Struct **ld;
  BD_Struct **bd;
  DBD_Struct **db;
  int mlevel;
  int cDBG;
  DBG_Struct **DBG;
  GHashTable *DBG_Table;
  char *ApsPath, *WfcPath, *RedPath, *DbPath, *DBLoggerPath, *DBMasterPath,
      *DBSlavePath;

  char *DBMasterLogDBName;
  char *CryptoPass;
} DI_Struct;

typedef struct {
  char window[SIZE_NAME + 1];
  char widget[SIZE_NAME + 1];
  char event[SIZE_NAME + 1];
  char uuid[SIZE_TERM + 1];
  char tenant[SIZE_TERM + 1];
  char host[SIZE_HOST + 1];
  char user[SIZE_USER + 1];
  char tempdir[SIZE_PATH + 1];
  char command;
  unsigned char dbstatus;
  unsigned char puttype;
} MessageHeader;

#endif
