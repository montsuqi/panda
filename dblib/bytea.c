#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <uuid/uuid.h>

#include "libmondai.h"
#include "directory.h"
#include "dbgroup.h"
#include "dbutils.h"
#include "monsys.h"
#include "option.h"
#include "enum.h"
#include "comm.h"
#include "bytea.h"
#include "message.h"
#include "debug.h"

#define DEFAULTSTATUS 403
#define DEFAULTCONTENT "application/octet-stream"

char *columns[][2] = {{"id", "varchar(37)"},
                      {"blobid", "int"},
                      {"importtime", "timestamp with time zone"},
                      {"lifetype", "int"},
                      {"filename", "varchar(4096)"},
                      {"size", "int"},
                      {"content_type", "varchar(1024)"},
                      {"status", "int"},
                      {"file_data", "bytea"},
                      {NULL, NULL}};

static char *get_columns(DBG_Struct *dbg, char *table_name) {
  char *sql;
  ValueStruct *retval = NULL;
  char *columns = NULL;

  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql,
          "SELECT array_to_string(array_agg(column_name::text),',') AS columns "
          "FROM pg_tables JOIN information_schema.columns on "
          "pg_tables.tablename = columns.table_name  WHERE table_name = '%s';",
          table_name);
  retval = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  if (retval) {
    if (IS_VALUE_RECORD(retval)) {
      columns = StrDup(ValueToString(ValueRecordItem(retval, 0), dbg->coding));
    }
    FreeValueStruct(retval);
  }
  xfree(sql);
  return columns;
}

static Bool check_id(char *id) {
  uuid_t u;

  if (id == NULL) {
    Warning("id is null\n");
    return FALSE;
  }
  if (uuid_parse(id, u) < 0) {
    Warning("[%s] is an invalid uuid\n", id);
    return FALSE;
  }
  return TRUE;
}

static Bool migration_monblob(DBG_Struct *dbg) {
  Bool rc;
  char *sql;
  char *table_name;
  char *columns;

  table_name = (char *)xmalloc(SIZE_BUFF);
  sprintf(table_name, "%s__bak", MONBLOB);
  columns = get_columns(dbg, table_name);
  if (columns == NULL) {
    return FALSE;
  }
  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql, "INSERT INTO %s (%s) SELECT %s FROM %s;", MONBLOB, columns,
          columns, table_name);
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  xfree(columns);
  xfree(table_name);
  return (rc == MCP_OK);
}

static Bool create_monblob(DBG_Struct *dbg) {
  Bool rc;
  char *sql, *p;
  int i;

  sql = (char *)xmalloc(SIZE_BUFF);
  p = sql;
  p += sprintf(p, "CREATE TABLE %s (", MONBLOB);
  for (i = 0; columns[i][0] != NULL; i++) {
    if (i != 0) {
      p += sprintf(p, ", ");
    }
    p += sprintf(p, "%s %s", columns[i][0], columns[i][1]);
  }
  sprintf(p, ");");
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  if (rc == MCP_OK) {
    sprintf(sql, "ALTER TABLE ONLY %s ADD CONSTRAINT %s_pkey PRIMARY KEY(id);",
            MONBLOB, MONBLOB);
    rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  }
  if (rc == MCP_OK) {
    sprintf(sql, "CREATE INDEX %s_blobid ON %s (blobid);", MONBLOB, MONBLOB);
    rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  }
  if (rc == MCP_OK) {
    sprintf(sql, "CREATE INDEX %s_importtime ON %s (importtime);", MONBLOB,
            MONBLOB);
    rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  }
  xfree(sql);
  return (rc == MCP_OK);
}

static Bool recreate_monblob(DBG_Struct *dbg) {
  int rc;
  char sql[SIZE_SQL + 1];

  if (table_exist(dbg, "monblob__bak") == TRUE) {
    sprintf(sql, "DROP TABLE %s__bak CASCADE;", MONBLOB);
    if (ExecDBOP(dbg, sql, FALSE, DB_UPDATE) != MCP_OK) {
      return FALSE;
    }
  }
  if (index_exist(dbg, "monblob_pkey") == TRUE) {
    sprintf(sql, "ALTER TABLE %s DROP CONSTRAINT %s_pkey;", MONBLOB, MONBLOB);
    if (ExecDBOP(dbg, sql, FALSE, DB_UPDATE) != MCP_OK) {
      return FALSE;
    }
  }
  sprintf(sql, "ALTER TABLE %s RENAME TO %s__bak;", MONBLOB, MONBLOB);
  if (ExecDBOP(dbg, sql, FALSE, DB_UPDATE) != MCP_OK) {
    return FALSE;
  }
  int i;
  char *index[] = {"monblob_blobid", "monblob_importtime", NULL};
  for (i = 0; index[i] != NULL; i++) {
    sprintf(sql, "DROP INDEX IF EXISTS %s;", index[i]);
    if (ExecDBOP(dbg, sql, FALSE, DB_UPDATE) != MCP_OK) {
      return FALSE;
    }
  }
  if (!create_monblob(dbg)) {
    return FALSE;
  }
  if (!migration_monblob(dbg)) {
    return FALSE;
  }
  sprintf(sql, "DROP TABLE %s__bak CASCADE;", MONBLOB);
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  return (rc == MCP_OK);
}

static Bool create_sequence(DBG_Struct *dbg) {
  char *sql;
  int rc;

  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql, "CREATE SEQUENCE %s;", SEQMONBLOB);
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  return (rc == MCP_OK);
}

extern Bool monblob_setup(DBG_Struct *dbg, Bool recreate) {
  int rc;

  TransactionStart(dbg);
  if (table_exist(dbg, MONBLOB) != TRUE) {
    create_monblob(dbg);
  }
  if ((column_exist(dbg, MONBLOB, "blobid") != TRUE) || recreate) {
    recreate_monblob(dbg);
  }
  if (sequence_exist(dbg, SEQMONBLOB) != TRUE) {
    create_sequence(dbg);
  }
  rc = TransactionEnd(dbg);
  return (rc == MCP_OK);
}

static Bool monblob_idcheck(DBG_Struct *dbg, char *id) {
  Bool rc;
  char *sql;
  char *eid;
  ValueStruct *ret;

  sql = (char *)xmalloc(SIZE_BUFF);
  eid = Escape_monsys(dbg, id);
  sprintf(sql, "SELECT 1 FROM %s WHERE id='%s';", MONBLOB, eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  if (ret) {
    rc = TRUE;
    FreeValueStruct(ret);
  } else {
    rc = FALSE;
  }
  return rc;
}

static void reset_blobid(DBG_Struct *dbg) {
  char *sql;
  ValueStruct *ret;

  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql, "SELECT setval('%s', 1, false);", SEQMONBLOB);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  FreeValueStruct(ret);
  xfree(sql);
}

extern MonObjectType new_blobid(DBG_Struct *dbg) {
  MonObjectType blobid;
  ValueStruct *ret, *val;
  char *sql;

  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql, "SELECT nextval('%s') AS id;", SEQMONBLOB);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  if (ret) {
    val = GetItemLongName(ret, "id");
    blobid = (MonObjectType)ValueToInteger(val);
    FreeValueStruct(ret);
  } else {
    blobid = 0;
  }
  xfree(sql);
  if ((unsigned int)blobid >= USHRT_MAX) {
    reset_blobid(dbg);
  }
  return blobid;
}

extern char *new_id() {
  uuid_t u;
  static char *id;

  id = xmalloc(SIZE_TERM + 1);
  uuid_generate(u);
  uuid_unparse(u, id);
  return id;
}

extern monblob_struct *new_monblob_struct(DBG_Struct *dbg, char *id,
                                          MonObjectType obj) {
  monblob_struct *monblob;

  monblob = New(monblob_struct);
  if (id == NULL) {
    monblob->id = new_id();
  } else {
    monblob->id = StrDup(id);
  }
  if (obj <= 0) {
    monblob->blobid = new_blobid(dbg);
  } else {
    monblob->blobid = obj;
  }
  monblob->lifetype = 0;
  monblob->filename = NULL;
  monblob->status = DEFAULTSTATUS;
  monblob->size = 0;
  monblob->content_type = StrDup(DEFAULTCONTENT);
  monblob->bytea = NULL;
  monblob->bytea_len = 0;
  return monblob;
}

void free_monblob_struct(monblob_struct *monblob) {
  if (!monblob) {
    return;
  }
  if (monblob->id) {
    xfree(monblob->id);
  }
  if (monblob->filename) {
    xfree(monblob->filename);
  }
  if (monblob->content_type) {
    xfree(monblob->content_type);
  }
  xfree(monblob);
}

extern ValueStruct *escape_bytea(DBG_Struct *dbg, char *src, size_t len) {
  ValueStruct *value, *retval;

  value = NewValue(GL_TYPE_BINARY);
  SetValueBinary(value, src, len);
  retval = ExecDBESCAPEBYTEA(dbg, NULL, NULL, value);
  FreeValueStruct(value);

  return retval;
}

extern ValueStruct *unescape_bytea(DBG_Struct *dbg, ValueStruct *value) {
  ValueStruct *retval;
  retval = ExecDBUNESCAPEBYTEA(dbg, NULL, NULL, value);
  return retval;
}

extern size_t file_to_bytea(DBG_Struct *dbg, char *filename,
                            ValueStruct **value) {
  struct stat stbuf;
  unsigned char buff[SIZE_BUFF];
  unsigned char *src, *src_p;
  FILE *fp;
  size_t fsize, bsize, left;

  if (stat(filename, &stbuf) != 0) {
    Warning("%s: %s\n", filename, strerror(errno));
    return 0;
  }
  if (S_ISDIR(stbuf.st_mode)) {
    Warning("%s: Is adirectory\n", filename);
    return 0;
  }
  fsize = stbuf.st_size;
  src = (unsigned char *)xmalloc(fsize);
  src_p = src;

  fp = fopen(filename, "rb");
  left = fsize;
  do {
    if (left > SIZE_BUFF) {
      bsize = SIZE_BUFF;
    } else {
      bsize = left;
    }
    bsize = fread(buff, 1, bsize, fp);
    memcpy(src_p, buff, bsize);
    src_p = src_p + bsize;
    if (bsize > 0) {
      left -= bsize;
    }
  } while (left > 0);
  fclose(fp);
  *value = escape_bytea(dbg, src, fsize);
  xfree(src);
  return (int)fsize;
}

static char *_monblob_import(DBG_Struct *dbg, char *id, int persist,
                             char *filename, char *content_type,
                             unsigned int lifetype, ValueStruct *value,
                             size_t size) {
  monblob_struct *monblob;

  if (value == NULL) {
    return NULL;
  }

  monblob = new_monblob_struct(dbg, id, 0);
  monblob->filename = StrDup(basename((char *)filename));
  monblob->lifetype = lifetype;
  if (persist > 0) {
    if (monblob->lifetype == 0) {
      monblob->lifetype = 1;
    }
  }
  if (monblob->lifetype > 2) {
    monblob->lifetype = 2;
  }
  timestamp(monblob->importtime, sizeof(monblob->importtime));
  if (content_type != NULL) {
    xfree(monblob->content_type);
    monblob->content_type = StrDup(content_type);
  }
  monblob->size = size;
  monblob->bytea = ValueToString(value, NULL);
  monblob->bytea_len = strlen(monblob->bytea);
  monblob->status = 200;
  monblob_insert(dbg, monblob, monblob_idcheck(dbg, monblob->id));
  if (monblob->id) {
    id = StrDup(monblob->id);
  }
  FreeValueStruct(value);
  free_monblob_struct(monblob);

  return (char *)id;
}

static Bool monblob_expire(DBG_Struct *dbg) {
  Bool rc;
  char *sql;
  size_t sql_len = SIZE_SQL;

  sql = (char *)xmalloc(sql_len);
  snprintf(sql, sql_len,
           "DELETE FROM %s WHERE (lifetype = 0 AND (now() > importtime + "
           "CAST('%d days' AS INTERVAL))) OR (lifetype = 1 AND (now() > "
           "importtime + CAST('%d days' AS INTERVAL)));",
           MONBLOB, BLOBEXPIRE, MONBLOBEXPIRE);
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  return (rc == MCP_OK);
}

extern char *monblob_import(DBG_Struct *dbg, char *id, int persist,
                            char *filename, char *content_type,
                            unsigned int lifetype) {
  ValueStruct *value = NULL;
  size_t size;

  /* Delete expiration date */
  monblob_expire(dbg);

  size = file_to_bytea(dbg, filename, &value);
  if (value == NULL) {
    return NULL;
  }
  return _monblob_import(dbg, id, persist, filename, content_type, lifetype,
                         value, size);
}

extern MonObjectType blob_import(DBG_Struct *dbg, int persist, char *filename,
                                 char *content_type, unsigned int lifetype) {
  char *id;
  MonObjectType blobid;

  id = monblob_import(dbg, NULL, persist, filename, content_type, lifetype);
  blobid = monblob_get_blobid(dbg, id);
  if (id != NULL) {
    xfree(id);
  }
  return blobid;
}

extern char *monblob_import_mem(DBG_Struct *dbg, char *id, int persist,
                                char *filename, char *content_type,
                                unsigned int lifetype, char *buf, size_t size) {
  ValueStruct *value;

  value = escape_bytea(dbg, buf, size);
  if (value == NULL) {
    return NULL;
  }
  return _monblob_import(dbg, id, persist, filename, content_type, lifetype,
                         value, size);
}

extern MonObjectType blob_import_mem(DBG_Struct *dbg, int persist,
                                     char *filename, char *content_type,
                                     unsigned int lifetype, char *buf,
                                     size_t size) {
  char *id;
  MonObjectType blobid;

  id = monblob_import_mem(dbg, NULL, persist, filename, content_type, lifetype,
                          buf, size);
  blobid = monblob_get_blobid(dbg, id);
  if (id != NULL) {
    xfree(id);
  }
  return blobid;
}

static size_t insert_query(DBG_Struct *dbg, monblob_struct *blob, char *query) {
  size_t size;
  char *filename;

  filename = Escape_monsys(dbg, blob->filename);
  size = snprintf(query, SIZE_SQL,
                  "INSERT INTO %s (id, blobid, importtime, lifetype, filename, "
                  "size, content_type, status, file_data) "
                  "VALUES ('%s', '%u', '%s', %d, '%s', %d, '%s', %d, '",
                  MONBLOB, blob->id, (unsigned int)blob->blobid,
                  blob->importtime, blob->lifetype, filename, blob->size,
                  blob->content_type, blob->status);
  xfree(filename);
  return size;
}

static size_t update_query(DBG_Struct *dbg, monblob_struct *blob, char *query) {
  size_t size;
  char *filename;

  filename = Escape_monsys(dbg, blob->filename);
  size = snprintf(
      query, SIZE_SQL,
      "UPDATE %s SET blobid = '%u', importtime = '%s', lifetype = %d, filename "
      "= '%s', size = %d,  content_type = '%s', status = %d, file_data = '",
      MONBLOB, (unsigned int)blob->blobid, blob->importtime, blob->lifetype,
      filename, blob->size, blob->content_type, blob->status);
  xfree(filename);
  return size;
}

extern Bool monblob_insert(DBG_Struct *dbg, monblob_struct *monblob,
                           Bool update) {
  char *sql, *sql_p;
  size_t sql_len = SIZE_SQL;
  int rc;
  sql = xmalloc(monblob->bytea_len + sql_len);
  sql_p = sql;
  if (update) {
    sql_p += update_query(dbg, monblob, sql_p);
  } else {
    sql_p += insert_query(dbg, monblob, sql_p);
  }
  strncpy(sql_p, monblob->bytea, monblob->bytea_len);
  sql_p += monblob->bytea_len;
  if (update) {
    sql_p += snprintf(sql_p, sql_len, "' WHERE id='%s'", monblob->id);
  } else {
    sql_p += snprintf(sql_p, sql_len, "')");
  }
  snprintf(sql_p, sql_len, ";");
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);

  return (rc == MCP_OK);
}

extern char *value_to_file(char *filename, ValueStruct *value) {
  FILE *fp;
  size_t size;

  if ((fp = fopen(filename, "wb")) == NULL) {
    Warning("fopen: %s: %s\n", strerror(errno), filename);
    return NULL;
  }
  size = fwrite(ValueByte(value), ValueByteLength(value), 1, fp);
  if (size < 1) {
    Warning("write error: %s\n", filename);
    return NULL;
  }
  if (fclose(fp) != 0) {
    Warning("fclose: %s: %s\n", strerror(errno), filename);
    return NULL;
  }
  return filename;
}

ValueStruct *monblob_export(DBG_Struct *dbg, char *id) {
  char *sql;
  char *eid;
  size_t sql_len = SIZE_SQL;
  ValueStruct *ret;

  sql = (char *)xmalloc(sql_len);
  eid = Escape_monsys(dbg, id);
  snprintf(sql, sql_len,
           "SELECT id, filename, content_type, status, file_data FROM %s WHERE "
           "id = '%s';",
           MONBLOB, eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  return ret;
}

extern Bool monblob_export_file(DBG_Struct *dbg, char *id, char *filename) {
  char *sql;
  char *eid;
  size_t sql_len = SIZE_SQL;
  ValueStruct *value, *ret, *retval;
  uuid_t u;
  Bool rc;

  if (filename == NULL) {
    Warning("filename is null\n");
    return FALSE;
  }
  if (id == NULL) {
    Warning("id is null\n");
    return FALSE;
  }
  if (uuid_parse(id, u) < 0) {
    Warning("[%s] is invalid\n", id);
    return FALSE;
  }

  sql = (char *)xmalloc(sql_len);
  eid = Escape_monsys(dbg, id);
  snprintf(sql, sql_len, "SELECT file_data FROM %s WHERE id = '%s';", MONBLOB,
           eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  if (!ret) {
    Warning("[%s] is not registered\n", id);
    return FALSE;
  }

  value = GetItemLongName(ret, "file_data");
  retval = unescape_bytea(dbg, value);
  rc = FALSE;
  if (value_to_file(filename, retval) != NULL) {
    rc = TRUE;
  }
  FreeValueStruct(retval);
  FreeValueStruct(ret);

  return rc;
}

extern Bool blob_export(DBG_Struct *dbg, MonObjectType blobid, char *filename) {
  char *id;
  Bool ret;

  id = monblob_get_id(dbg, blobid);
  if (id == NULL) {
    Warning("object[%d] not found\n", (unsigned int)blobid);
    return FALSE;
  }
  ret = monblob_export_file(dbg, id, filename);
  monblob_delete(dbg, id);
  xfree(id);
  return ret;
}

extern Bool monblob_export_mem(DBG_Struct *dbg, char *id, char **buf,
                               size_t *size) {
  char *sql;
  char *eid;
  size_t sql_len = SIZE_SQL;
  ValueStruct *value, *ret, *retval;

  if (!check_id(id)) {
    return FALSE;
  }

  sql = (char *)xmalloc(sql_len);
  eid = Escape_monsys(dbg, id);
  snprintf(sql, sql_len, "SELECT file_data FROM %s WHERE id = '%s';", MONBLOB,
           eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  if (!ret) {
    Warning("[%s] is not registered\n", id);
    return FALSE;
  }
  value = GetItemLongName(ret, "file_data");
  retval = unescape_bytea(dbg, value);
  *size = ValueByteLength(retval);
  *buf = xmalloc(*size);
  memcpy(*buf, ValueByte(retval), *size);
  FreeValueStruct(retval);
  FreeValueStruct(ret);

  return TRUE;
}

extern Bool blob_export_mem(DBG_Struct *dbg, MonObjectType blobid, char **buf,
                            size_t *size) {
  char *id;
  Bool ret;

  id = monblob_get_id(dbg, blobid);
  if (id == NULL) {
    Warning("object[%d] not found\n", (unsigned int)blobid);
    return FALSE;
  }
  ret = monblob_export_mem(dbg, id, buf, size);
  monblob_delete(dbg, id);
  xfree(id);
  return ret;
}

static Bool monblob_update(DBG_Struct *dbg, monblob_struct *monblob) {
  Bool rc;
  char *sql, *sql_p;
  char *filename, *content_type, *id;
  size_t sql_len = SIZE_SQL;

  sql = xmalloc(sql_len);
  sql_p = sql;
  sql_p += snprintf(sql_p, sql_len, "UPDATE %s SET lifetype = %d, ", MONBLOB,
                    monblob->lifetype);
  if ((monblob->filename != NULL) && (strlen(monblob->filename) > 0)) {
    filename = Escape_monsys(dbg, monblob->filename);
    sql_p += snprintf(sql_p, sql_len, "filename = '%s', ", filename);
    xfree(filename);
  }
  if ((monblob->content_type != NULL) && (strlen(monblob->content_type) > 0)) {
    content_type = Escape_monsys(dbg, monblob->content_type);
    sql_p += snprintf(sql_p, sql_len, "content_type = '%s', ",
                      monblob->content_type);
    xfree(content_type);
  }
  sql_p += snprintf(sql_p, sql_len, "status = %d ", monblob->status);
  id = Escape_monsys(dbg, monblob->id);
  sql_p += snprintf(sql_p, sql_len, "WHERE id = '%s'", id);
  xfree(id);
  snprintf(sql_p, sql_len, ";");
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  return (rc == MCP_OK);
}

extern int monblob_persist(DBG_Struct *dbg, char *id, char *filename,
                           char *content_type, unsigned int lifetype) {
  int rc;
  monblob_struct *monblob;

  monblob = new_monblob_struct(dbg, id, 0);
  monblob->filename = StrDup(filename);
  monblob->lifetype = lifetype;
  if (monblob->lifetype == 0) {
    monblob->lifetype = 1;
  }
  if (monblob->lifetype > 2) {
    monblob->lifetype = 2;
  }
  monblob->content_type = StrDup(content_type);
  monblob->status = 200;
  rc = monblob_update(dbg, monblob);
  free_monblob_struct(monblob);
  return rc;
}

/* update only lifetime */
extern Bool blob_persist(DBG_Struct *dbg, MonObjectType blobid) {
  Bool rc;
  monblob_struct *monblob;
  char *id;

  id = monblob_get_id(dbg, blobid);
  if (id != NULL) {
    monblob = new_monblob_struct(dbg, id, 0);
    if (monblob->lifetype <= 0) {
      monblob->lifetype = 1;
    }
    if (monblob->lifetype > 2) {
      monblob->lifetype = 2;
    }
    monblob->status = 200;
    rc = monblob_update(dbg, monblob);
    free_monblob_struct(monblob);
    xfree(id);
  } else {
    rc = FALSE;
  }
  return rc;
}

extern Bool monblob_delete(DBG_Struct *dbg, char *id) {
  Bool rc;
  char *sql;
  size_t sql_len = SIZE_SQL;

  sql = (char *)xmalloc(sql_len);
  snprintf(sql, sql_len, "DELETE FROM %s WHERE id = '%s'", MONBLOB, id);
  rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  return (rc == MCP_OK);
}

extern Bool blob_delete(DBG_Struct *dbg, MonObjectType blobid) {
  Bool rc;
  char *id;

  id = monblob_get_id(dbg, blobid);
  if (id != NULL) {
    rc = monblob_delete(dbg, id);
    xfree(id);
  } else {
    rc = FALSE;
  }
  return rc;
}

extern char *monblob_get_filename(DBG_Struct *dbg, char *id) {
  char *sql;
  char *eid;
  char *filename = NULL;
  ValueStruct *ret, *val;

  sql = (char *)xmalloc(SIZE_BUFF);
  eid = Escape_monsys(dbg, id);
  sprintf(sql, "SELECT filename FROM %s WHERE id = '%s';", MONBLOB, eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  if (ret) {
    val = GetItemLongName(ret, "filename");
    filename = StrDup(ValueToString(val, dbg->coding));
  }
  FreeValueStruct(ret);
  return filename;
}

extern char *monblob_get_id(DBG_Struct *dbg, MonObjectType blobid) {
  char *sql;
  ValueStruct *ret, *val;
  char *id = NULL;

  if (blobid == 0) {
    return NULL;
  }
  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql,
          "SELECT id FROM %s WHERE blobid = %u AND now() < importtime + "
          "CAST('%d days' AS INTERVAL) ORDER BY importtime DESC LIMIT 1;",
          MONBLOB, (unsigned int)blobid, BLOBEXPIRE);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  if (ret == NULL) {
    return NULL;
  }
  val = GetItemLongName(ret, "id");
  if (val) {
    id = StrDup(ValueToString(val, dbg->coding));
  } else {
    Warning("[%s] is not registered\n", id);
  }
  FreeValueStruct(ret);
  return id;
}

extern MonObjectType monblob_get_blobid(DBG_Struct *dbg, char *id) {
  char *sql;
  char *eid;
  ValueStruct *ret, *val;
  MonObjectType blobid;

  blobid = 0;
  if (id == NULL) {
    return 0;
  }
  sql = (char *)xmalloc(SIZE_BUFF);
  eid = Escape_monsys(dbg, id);
  sprintf(sql, "SELECT blobid FROM %s WHERE id = '%s';", MONBLOB, eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  if (ret) {
    val = GetItemLongName(ret, "blobid");
    blobid = (MonObjectType)ValueToInteger(val);
    FreeValueStruct(ret);
  }
  return blobid;
}

extern Bool monblob_check_id(DBG_Struct *dbg, char *id) {
  char *sql;
  char *eid;
  ValueStruct *ret;

  if (!check_id(id)) {
    return FALSE;
  }
  sql = (char *)xmalloc(SIZE_BUFF);
  eid = Escape_monsys(dbg, id);
  sprintf(sql, "SELECT id FROM %s WHERE id = '%s' and status = 200;", MONBLOB,
          eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  if (ret) {
    FreeValueStruct(ret);
    return TRUE;
  } else {
    return FALSE;
  }
}

extern Bool blob_check_id(DBG_Struct *dbg, MonObjectType blobid) {
  char *sql;
  ValueStruct *ret;

  sql = (char *)xmalloc(SIZE_BUFF);
  sprintf(sql, "SELECT id FROM %s WHERE blobid = '%u' and status = 200;",
          MONBLOB, (unsigned int)blobid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  if (ret) {
    FreeValueStruct(ret);
    return TRUE;
  } else {
    return FALSE;
  }
}

extern ValueStruct *blob_list(DBG_Struct *dbg) {
  char *sql;
  size_t sql_len = SIZE_SQL;
  ValueStruct *ret;

  sql = (char *)xmalloc(sql_len);
  snprintf(
      sql, sql_len,
      "SELECT importtime, id, blobid, "
      "filename,size,content_type,lifetype,status FROM %s ORDER BY importtime;",
      MONBLOB);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(sql);
  return ret;
}

extern ValueStruct *monblob_info(DBG_Struct *dbg, char *id) {
  char *sql;
  char *eid;
  size_t sql_len = SIZE_SQL;
  ValueStruct *ret;

  if (!check_id(id)) {
    return NULL;
  }
  sql = (char *)xmalloc(sql_len);
  eid = Escape_monsys(dbg, id);
  snprintf(
      sql, sql_len,
      "SELECT importtime, id, blobid, "
      "filename,size,content_type,lifetype,status FROM %s WHERE id = '%s';",
      MONBLOB, eid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eid);
  xfree(sql);
  return ret;
}

extern ValueStruct *blob_info(DBG_Struct *dbg, char *blobid) {
  char *sql;
  char *eblobid;
  size_t sql_len = SIZE_SQL;
  ValueStruct *ret;

  sql = (char *)xmalloc(sql_len);
  eblobid = Escape_monsys(dbg, blobid);
  snprintf(
      sql, sql_len,
      "SELECT importtime, id, blobid, "
      "filename,size,content_type,lifetype,status FROM %s WHERE blobid = '%s';",
      MONBLOB, eblobid);
  ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
  xfree(eblobid);
  xfree(sql);
  return ret;
}
