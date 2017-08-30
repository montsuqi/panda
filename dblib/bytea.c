#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<libgen.h>
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<signal.h>
#include	<time.h>
#include	<errno.h>

#include	<uuid/uuid.h>

#include	"libmondai.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"dbutils.h"
#include	"monsys.h"
#include	"option.h"
#include	"enum.h"
#include	"comm.h"
#include	"bytea.h"

#define DEFAULTSTATUS 403
#define DEFAULTCONTENT "application/octet-stream"

char *columns[][2] = {\
	{"id", "varchar(37)"},
	{"blobid", "int"},
	{"importtime", "timestamp with time zone"},
	{"lifetype", "int"},
	{"filename", "varchar(4096)"},
	{"size", "int"},
	{"content_type", "varchar(1024)"},
	{"status", "int"},
	{"file_data", "bytea"},
	{NULL, NULL}
};

static char *
get_columns(
		DBG_Struct      *dbg,
		char *table_name)
{
	char *sql;
	ValueStruct	*retval = NULL;
	char *columns = NULL;

	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "SELECT array_to_string(array_agg(column_name::text),',') AS columns FROM pg_tables JOIN information_schema.columns on pg_tables.tablename = columns.table_name  WHERE table_name = '%s';", table_name);
	retval = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	if (ValueType(retval) == GL_TYPE_RECORD) {
		columns = StrDup(ValueToString(ValueRecordItem(retval,0), dbg->coding));
	}
	FreeValueStruct(retval);
	xfree(sql);
	return columns;
}

static Bool
migration_monblob(
        DBG_Struct      *dbg)
{
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
	sprintf(sql, "INSERT INTO %s (%s) SELECT %s FROM %s;", MONBLOB, columns, columns, table_name);
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	xfree(sql);
	xfree(columns);
	xfree(table_name);
	return (rc == MCP_OK);
}

static Bool
create_monblob(
        DBG_Struct      *dbg)
{
	Bool rc;
	char *sql, *p;
	int i;

	sql = (char *)xmalloc(SIZE_BUFF);
	p = sql;
	p += sprintf(p, "CREATE TABLE %s (", MONBLOB);
	for (i = 0; columns[i][0] != NULL; i++) {
		if (i == 0) {
			p += sprintf(p, "%s %s primary key",columns[i][0],columns[i][1]);
		} else {
			p += sprintf(p, ", ");
			p += sprintf(p, "%s %s",columns[i][0],columns[i][1]);
		}
	}
	sprintf(p, ");");
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	if (rc == MCP_OK) {
		sprintf(sql, "CREATE INDEX %s_blobid ON %s (blobid);",MONBLOB, MONBLOB);
		rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	}
	if (rc == MCP_OK) {
		sprintf(sql, "CREATE INDEX %s_importime ON %s (importtime);",MONBLOB, MONBLOB);
		rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	}
	xfree(sql);
	return (rc == MCP_OK);
}

static Bool
recreate_monblob(
        DBG_Struct      *dbg)
{
	int rc;
	char	sql[SIZE_SQL+1];

	if ( table_exist(dbg, "monblob__bak") == TRUE) {
		sprintf(sql, "DROP TABLE %s__bak;", MONBLOB);
		if (ExecDBOP(dbg, sql, TRUE, DB_UPDATE) != MCP_OK) {
			return FALSE;
		}
	}
	sprintf(sql, "ALTER TABLE %s RENAME TO %s__bak;", MONBLOB, MONBLOB);
	if (ExecDBOP(dbg, sql, TRUE, DB_UPDATE) != MCP_OK) {
		return FALSE;
	}
	if (!create_monblob(dbg)) {
		return FALSE;
	}
	if (!migration_monblob(dbg)) {
		return FALSE;
	}
	sprintf(sql, "DROP TABLE %s__bak;", MONBLOB);
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	return (rc == MCP_OK);
}

static Bool
create_sequence(
        DBG_Struct      *dbg)
{
	char *sql;
	int rc;

	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "CREATE SEQUENCE %s;", SEQMONBLOB);
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	xfree(sql);
	return (rc == MCP_OK);
}

extern Bool
monblob_setup(
	DBG_Struct	*dbg)
{
	int 	rc;

	TransactionStart(dbg);
	if ( table_exist(dbg, MONBLOB) != TRUE) {
		create_monblob(dbg);
	}
	if ( column_exist(dbg, MONBLOB, "blobid") != TRUE ) {
		recreate_monblob(dbg);
	}
	if ( sequence_exist(dbg, SEQMONBLOB) != TRUE) {
		create_sequence(dbg);
	}
	rc = TransactionEnd(dbg);
	return (rc == MCP_OK);
}

static Bool
monblob_idcheck(
	DBG_Struct	*dbg,
	char *id)
{
	Bool rc;
	char *sql;
	ValueStruct *ret;

	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "SELECT 1 FROM %s WHERE id='%s';", MONBLOB, id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (ret) {
		rc = TRUE;
		FreeValueStruct(ret);
	} else {
		rc = FALSE;
	}
	return rc;

}

static void
reset_blobid(
		DBG_Struct	*dbg)
{
	char *sql;
	ValueStruct *ret;

	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "SELECT setval('%s', 1, false);", SEQMONBLOB);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	FreeValueStruct(ret);
}

extern MonObjectType
new_blobid(
		DBG_Struct	*dbg)
{
	MonObjectType oid;
	ValueStruct	*ret, *val;
	char *sql;

	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "SELECT nextval('%s') AS id;", SEQMONBLOB);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	if (ret) {
		val = GetItemLongName(ret,"id");
		oid = (MonObjectType )ValueToInteger(val);
		FreeValueStruct(ret);
	} else {
		oid = 0;
	}
	xfree(sql);
	if ((unsigned int)oid >= USHRT_MAX) {
		reset_blobid(dbg);
	}
	return oid;
}

extern char *
new_id()
{
	uuid_t	u;
	static	char *id;

	id = xmalloc(SIZE_TERM+1);
	uuid_generate(u);
	uuid_unparse(u, id);
	return id;
}

extern monblob_struct *
NewMonblob_struct(
	DBG_Struct	*dbg,
	char *id,
	MonObjectType	obj)
{
	monblob_struct *monblob;

	monblob = New(monblob_struct);
	if (id == NULL) {
		monblob->id = new_id();
	} else {
		monblob->id = StrDup(id);
	}
	if ( obj <= 0 ) {
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

void
FreeMonblob_struct(
	monblob_struct *monblob)
{
	if (!monblob){
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

extern ValueStruct *
escape_bytea(
	DBG_Struct	*dbg,
	unsigned char *src,
	size_t len)
{
	ValueStruct	*value, *retval;

	value = NewValue(GL_TYPE_BINARY);
	SetValueBinary(value, src, len);
	retval = ExecDBESCAPEBYTEA(dbg, NULL, NULL, value);
	FreeValueStruct(value);

	return retval;
}

extern ValueStruct *
unescape_bytea(
	DBG_Struct	*dbg,
	ValueStruct	*value)
{
	ValueStruct *retval;
	retval = ExecDBUNESCAPEBYTEA(dbg, NULL, NULL, value);
	return retval;
}

extern int
file_to_bytea(
	DBG_Struct	*dbg,
	char	*filename,
	ValueStruct **value)
{
	struct	stat	stbuf;
	unsigned char	buff[SIZE_BUFF];
	unsigned char	*src, *src_p;
	FILE	*fp;
	size_t	fsize
		,   bsize
		,	left;

	if		(  stat(filename,&stbuf) != 0  ) {
		fprintf(stderr,"%s: %s\n", filename, strerror(errno));
		return 0;
	}
	if (S_ISDIR(stbuf.st_mode)) {
		fprintf(stderr,"%s: Is adirectory\n", filename);
		return 0;
	}
	fsize = stbuf.st_size;
	src = (unsigned char *)xmalloc(fsize);
	src_p = src;

	fp = fopen(filename,"rb");
	left = fsize;
	do {
		if		(  left  >  SIZE_BUFF  ) {
			bsize = SIZE_BUFF;
		} else {
			bsize = left;
		}
		bsize = fread(buff,1,bsize,fp);
		memcpy(src_p, buff, bsize);
		src_p = src_p + bsize;
		if		(  bsize  >  0  ) {
			left -= bsize;
		}
	}	while	(  left  >  0  );
	fclose(fp);
	*value = escape_bytea(dbg, src, fsize);
	xfree(src);
	return (int )fsize;
}

extern	char *
monblob_import(
	DBG_Struct	*dbg,
	char *id,
	int persist,
	char *filename,
	char *content_type,
	unsigned int lifetype)
{
	monblob_struct *monblob;
	ValueStruct *value = NULL;

	monblob = NewMonblob_struct(dbg, id, 0);
	monblob->filename = StrDup(basename(filename));
	monblob->lifetype = lifetype;
	if (persist > 0 ) {
		monblob->status = 200;
		if (monblob->lifetype == 0) {
			monblob->lifetype = 1;
		}
	} else {
		monblob->status = DEFAULTSTATUS;
	}
	if (monblob->lifetype > 2) {
		monblob->lifetype = 2;
	}
	timestamp(monblob->importtime, sizeof(monblob->importtime));
	if (content_type != NULL) {
		xfree(monblob->content_type);
		monblob->content_type = StrDup(content_type);
	}
	monblob->size = file_to_bytea(dbg, filename, &value);
	if (value == NULL){
		FreeMonblob_struct(monblob);
		return NULL;
	}
	monblob->bytea = ValueToString(value,NULL);
	monblob->bytea_len = strlen(monblob->bytea);
	monblob_insert(dbg, monblob, monblob_idcheck(dbg, monblob->id));
	if (monblob->id) {
		id = StrDup(monblob->id);
	}
	FreeValueStruct(value);
	FreeMonblob_struct(monblob);

	return id;
}

static size_t
insert_query(
	DBG_Struct	*dbg,
	monblob_struct *blob,
	char *query)
{
	size_t 	size;
	char *filename;

	filename = Escape_monsys(dbg, blob->filename);
	size = snprintf(query, SIZE_SQL, \
		   "INSERT INTO %s (id, blobid, importtime, lifetype, filename, size, content_type, status, file_data) "\
					"VALUES ('%s', '%u', '%s', %d, '%s', %d, '%s', %d, '", \
					MONBLOB, blob->id, (unsigned int)blob->blobid, blob->importtime, blob->lifetype, filename, blob->size,
					blob->content_type, blob->status );
	xfree(filename);
	return size;
}

static size_t
update_query(
	DBG_Struct	*dbg,
	monblob_struct *blob,
	char *query)
{
	size_t 	size;
	char *filename;

	filename = Escape_monsys(dbg, blob->filename);
	size = snprintf(query, SIZE_SQL, \
		   "UPDATE %s SET blobid = '%u', importtime = '%s', lifetype = %d, filename = '%s', size = %d,  content_type = '%s', status = %d, file_data = '", \
					MONBLOB, (unsigned int)blob->blobid, blob->importtime, blob->lifetype, filename, blob->size, blob->content_type, blob->status );
	xfree(filename);
	return size;
}

extern Bool
monblob_insert(
	DBG_Struct	*dbg,
	monblob_struct *monblob,
	Bool update)
{
	char	*sql, *sql_p;
	size_t	sql_len = SIZE_SQL;
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

extern char *
value_to_file(
	char *filename,
	ValueStruct	*value)
{
	FILE	*fp;
	size_t	size;

	if ((fp = fopen(filename,"wb")) == NULL ) {
		fprintf(stderr,"%s: %s\n", strerror(errno), filename);
		return NULL;
	}
	size = fwrite(ValueByte(value),ValueByteLength(value),1,fp);
	if ( size < 1) {
		fprintf(stderr,"write error: %s\n",  filename);
		return NULL;
	}
	if (fclose(fp) != 0) {
		fprintf(stderr,"%s: %s\n", strerror(errno), filename);
		return NULL;
	}
	return filename;
}

extern	char *
monblob_export(
	DBG_Struct *dbg,
	char *id,
	char *filename)
{
	char	*sql;
	size_t	sql_len = SIZE_SQL;
	ValueStruct	*value, *ret, *retval;
	uuid_t u;

	if (uuid_parse(id, u) < 0) {
		fprintf(stderr,"[%s] is invalid\n", id);
		return NULL;
	}

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT file_data FROM %s WHERE id = '%s'", MONBLOB, id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (!ret) {
		fprintf(stderr,"[%s] is not registered\n", id);
		return NULL;
	}
	value = GetItemLongName(ret, "file_data");
	retval = unescape_bytea(dbg, value);
	filename = value_to_file(filename, retval);
	FreeValueStruct(retval);
	FreeValueStruct(ret);
	return filename;
}

static void
monblob_update(
	DBG_Struct	*dbg,
	monblob_struct *monblob)
{
	char	*sql, *sql_p;
	size_t	sql_len = SIZE_SQL;
	sql = xmalloc(sql_len);
	sql_p = sql;
	sql_p += snprintf(sql_p, sql_len, "UPDATE %s SET lifetype = %d, ", MONBLOB, monblob->lifetype);
	if ((monblob->filename != NULL) && (strlen(monblob->filename) > 0 )){
		sql_p += snprintf(sql_p, sql_len, "filename = '%s', ",  monblob->filename);
	}
	if ((monblob->content_type != NULL) && (strlen(monblob->content_type) > 0 )){
		sql_p += snprintf(sql_p, sql_len, "content_type = '%s', ", monblob->content_type);
	}
	sql_p += snprintf(sql_p, sql_len, "status = %d ", monblob->status);
	sql_p += snprintf(sql_p, sql_len, "WHERE id = '%s'", monblob->id);
	snprintf(sql_p, sql_len, ";");
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
}

extern	void
monblob_persist(
	DBG_Struct	*dbg,
	char *id,
	char *filename,
	char *content_type,
	unsigned int lifetype)
{
	monblob_struct *monblob;

	monblob = NewMonblob_struct(dbg, id, 0);
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
	monblob_update(dbg, monblob);
	FreeMonblob_struct(monblob);
	return;
}

extern	void
monblob_delete(
	DBG_Struct	*dbg,
	char *id)
{
	char	*sql;
	size_t	sql_len = SIZE_SQL;

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "DELETE FROM %s WHERE id = '%s'", MONBLOB, id);
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
}

extern	char *
monblob_getfilename(
	DBG_Struct *dbg,
	char *id)
{
	char *sql;
	char *filename = NULL;
	ValueStruct	*ret, *val;

	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "SELECT filename FROM %s WHERE id = '%s';", MONBLOB, id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (ret) {
		val = GetItemLongName(ret, "filename");
		filename = StrDup(ValueToString(val,dbg->coding));
	}
	FreeValueStruct(ret);
	return filename;
}

extern	char *
monblob_getid(
	DBG_Struct *dbg,
	MonObjectType blobid)
{
	char *sql;
	ValueStruct	*ret, *val;
	char *id = NULL;

	if (blobid == 0){
		return NULL;
	}
	sql = (char *)xmalloc(SIZE_BUFF);
	sprintf(sql, "SELECT id FROM %s WHERE blobid = %u AND now() < importtime + CAST('%d days' AS INTERVAL);", MONBLOB, (unsigned int)blobid, BLOBEXPIRE);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (ret) {
		val = GetItemLongName(ret,"id");
		id = StrDup(ValueToString(val,dbg->coding));
		FreeValueStruct(ret);
	} else {
		fprintf(stderr,"[%s] is not registered\n", id);
	}
	return id;
}
