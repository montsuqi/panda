#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
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
	char *sql, *p;
	char *table_name;
	char *columns;

	table_name = (char *)xmalloc(SIZE_BUFF);
	sprintf(table_name, "%s__bak", MONBLOB);
	columns = get_columns(dbg, table_name);
	if (columns == NULL) {
		return FALSE;
	}
	sql = (char *)xmalloc(SIZE_BUFF);
	p = sql;
	p += sprintf(p, "INSERT INTO %s (%s) SELECT %s FROM %s;", MONBLOB, columns, columns, table_name);
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
	p += sprintf(p, ");");
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
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
	if (ret) {
		rc = TRUE;
		FreeValueStruct(ret);
	} else {
		rc = FALSE;
	}
	return rc;

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
	char *id)
{
	monblob_struct *monblob;

	monblob = New(monblob_struct);
	if (id == NULL) {
		monblob->id = new_id();
	} else {
		monblob->id = StrDup(id);
	}
	monblob->blobid = 0;
	monblob->lifetype = 0;
	monblob->filename = "";
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
	ValueStruct	*value, *recval, *retval, *tmpval;

	value = NewValue(GL_TYPE_BINARY);
	SetValueBinary(value, src, len);

	recval = NewValue(GL_TYPE_RECORD);
	ValueAddRecordItem(recval, "dbescapebytea", value);
	tmpval = ExecDBESCAPEBYTEA(dbg, NULL, NULL, recval);
	retval = DuplicateValue(GetItemLongName(tmpval,"dbescapebytea"),TRUE);
	FreeValueStruct(tmpval);
	FreeValueStruct(recval);
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
		fprintf(stderr,"%s: %s\n",  filename, strerror(errno));
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
	char *filename,
	char *content_type,
	unsigned int lifetype)
{
	monblob_struct *monblob;
	ValueStruct *value = NULL;

	monblob = NewMonblob_struct(id);
	monblob->filename = filename;
	monblob->lifetype = lifetype;
	if (monblob->lifetype > 2) {
		monblob->lifetype = 2;
	}
	timestamp(monblob->importtime, sizeof(monblob->importtime));
	if (content_type == NULL) {
		monblob->content_type = StrDup(DEFAULTCONTENT);
	} else {
		monblob->content_type = StrDup(content_type);
	}
	monblob->size = file_to_bytea(dbg, monblob->filename, &value);
	if (value == NULL){
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
					"VALUES ('%s', '%d', '%s', %d, '%s', %d, '%s', %d, '", \
					MONBLOB, blob->id, blob->blobid, blob->importtime, blob->lifetype, filename, blob->size,
					blob->content_type, DEFAULTSTATUS );
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
		   "UPDATE %s SET blobid = '%d', importtime = '%s', lifetype = %d, filename = '%s', size = %d,  content_type = '%s', status = %d, file_data = '", \
					MONBLOB, blob->blobid, blob->importtime, blob->lifetype, filename, blob->size, blob->content_type, DEFAULTSTATUS );
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
		sql_p = sql_p + update_query(dbg, monblob, sql_p);
	} else {
		sql_p = sql_p + insert_query(dbg, monblob, sql_p);
	}
	strncpy(sql_p, monblob->bytea, monblob->bytea_len);
	sql_p = sql_p + monblob->bytea_len;
	if (update) {
		snprintf(sql_p, sql_len, "' WHERE id='%s';", monblob->id);
	} else {
		snprintf(sql_p, sql_len, "');");
	}
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

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT file_data FROM monblob WHERE id = '%s'", id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (!ret) {
		fprintf(stderr,"[%s] is not registered\n", id);
		return NULL;
	}
	value = GetItemLongName(ret, "file_data");
	retval = unescape_bytea(dbg, value);
	filename = value_to_file(filename, retval);
	FreeValueStruct(ret);
	return filename;
}

