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


extern ValueStruct *
file_to_bytea(
	DBG_Struct	*dbg,
	char	*filename)
{
	struct	stat	stbuf;
	unsigned char	buff[SIZE_BUFF];
	unsigned char	*src, *src_p;
	ValueStruct *value = NULL;
	FILE	*fp;
	size_t	fsize
		,   bsize
		,	left;

	if		(  stat(filename,&stbuf) != 0  ) {
		fprintf(stderr,"%s: %s\n",  filename, strerror(errno));
		return NULL;
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
	value = escape_bytea(dbg, src, fsize);
	xfree(src);
	return value;
}

extern Bool
monblob_insert(
	DBG_Struct	*dbg,
	monblob_struct *blob)
{
	char	*sql, *sql_p;
	size_t 	size;
	size_t	sql_len = SIZE_SQL;
	int rc;

	sql = xmalloc(blob->bytea_len + sql_len);
	sql_p = sql;
	size = snprintf(sql_p, sql_len, \
		   "INSERT INTO monblob \
                        (id, importtime, lifetype, filename, file_data) \
                 VALUES ('%s', '%s', %d, '%s', '", blob->id, blob->importtime, blob->lifetype, blob->filename);
	sql_p = sql_p + size;
	strncpy(sql_p, blob->bytea, blob->bytea_len);
	sql_p = sql_p + blob->bytea_len;
	snprintf(sql_p, sql_len, "');");
	TransactionStart(dbg);
	rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	TransactionEnd(dbg);
	xfree(sql);

	return (rc == MCP_OK);
}
