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

char *columns[][2] = {\
	{"id", "varchar(37)"},
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
	if ( column_exist(dbg, MONBLOB, "status") != TRUE ) {
		recreate_monblob(dbg);
	}
	rc = TransactionEnd(dbg);
	return (rc == MCP_OK);
}

extern char *
new_blobid()
{
	uuid_t	u;
	static	char *id;

	id = xmalloc(SIZE_TERM+1);
	uuid_generate(u);
	uuid_unparse(u, id);
	return id;
}

extern monblob_struct *
NewMonblob_struct()
{
	monblob_struct *monblob;
	monblob = New(monblob_struct);
	monblob->id = new_blobid();
	monblob->lifetype = 0;
	monblob->filename = "";
	monblob->size = 0;
	monblob->bytea = NULL;
	monblob->bytea_len = 0;
	return monblob;
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
	ValueStruct	*recval, *tmpval, *retval;

	recval = NewValue(GL_TYPE_RECORD);
	ValueAddRecordItem(recval, "dbunescapebytea", value);
	tmpval = ExecDBUNESCAPEBYTEA(dbg, NULL, NULL, recval);
	retval = DuplicateValue(GetItemLongName(tmpval,"dbunescapebytea"),TRUE);
	FreeValueStruct(tmpval);
	FreeValueStruct(recval);
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

static size_t
monblob_query(
	DBG_Struct	*dbg,
	monblob_struct *blob,
	char *query)
{
	size_t 	size;
	char *filename;

	filename = Escape_monsys(dbg, blob->filename);
	size = snprintf(query, SIZE_SQL, \
		   "INSERT INTO %s (id, importtime, lifetype, filename, size, file_data) VALUES ('%s', '%s', %d, '%s', '%d', '", \
					MONBLOB, blob->id, blob->importtime, blob->lifetype, filename, blob->size);
	xfree(filename);
	return size;
}

extern Bool
monblob_insert(
	DBG_Struct	*dbg,
	monblob_struct *blob)
{
	char	*sql, *sql_p;
	size_t	sql_len = SIZE_SQL;
	int rc;

	sql = xmalloc(blob->bytea_len + sql_len);
	sql_p = sql;
	sql_p = sql_p + monblob_query(dbg, blob, sql_p);
	strncpy(sql_p, blob->bytea, blob->bytea_len);
	sql_p = sql_p + blob->bytea_len;
	snprintf(sql_p, sql_len, "');");
	TransactionStart(dbg);
	rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	TransactionEnd(dbg);
	xfree(sql);

	return (rc == MCP_OK);
}
