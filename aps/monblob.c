#define	MAIN

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
#include	"bytea.h"
#include	"option.h"
#include	"enum.h"
#include	"gettext.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;
static	char	*ImportFile;
static	char	*ExportID;
static	Bool	List;
static	char	*DeleteID;

static	char	*OutputFile;
static	unsigned int	LifeType;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"import",	STRING,		TRUE,	(void*)&ImportFile,
		"Import file name"								},
	{	"export",	STRING,		TRUE,	(void*)&ExportID,
		"Export ID"										},
	{	"list",		BOOLEAN,	TRUE,	(void*)&List,
		"list of imported file"							},
	{	"delete",	STRING,		TRUE,	(void*)&DeleteID,
		"Delete imported file"							},
	{	"output",	STRING,		TRUE,	(void*)&OutputFile,
		"output file name"								},
	{	"lifetype",	INTEGER,	TRUE,	(void*)&LifeType,
		"lifetime type"									},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory = NULL;
	ImportFile = NULL;
	ExportID = NULL;
	OutputFile = NULL;
	LifeType = 0;
	fTimer = TRUE;
}

static	void
InitSystem(void)
{
	char *dir;
	InitMessage("monblob",NULL);
	if ( (dir = getenv("MON_DIRECTORY_PATH")) != NULL ) {
		Directory = dir;
	}
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,P_NONE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
}

static         Bool
create_monblob(
        DBG_Struct      *dbg)
{
	Bool rc;
	char *sql, *p;
	sql = (char *)xmalloc(SIZE_BUFF);
	p = sql;
	p += sprintf(p, "CREATE TABLE monblob (");
	p += sprintf(p, "       id        varchar(37)  primary key,");
	p += sprintf(p, "       importtime  timestamp  with time zone,");
	p += sprintf(p, "       lifetype    int,");
	p += sprintf(p, "       filename    varchar(4096),");
	p += sprintf(p, "       file_data   bytea");
	p += sprintf(p, ");");
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	xfree(sql);
	return rc;
}

extern Bool
monblob_setup(
	DBG_Struct	*dbg)
{
	Bool	exist;
	int 	rc;
	TransactionStart(dbg);
	exist = table_exist(dbg, "monblob");
	if ( !exist ) {
		create_monblob(dbg);
	}
	rc = TransactionEnd(dbg);
	return (rc == MCP_OK);
}

static char *
file_import(
	DBG_Struct	*dbg,
	char *filename,
	unsigned int lifetype)
{
	char	*sql, *sql_p;
	char	*bytea;
	static	char *id;
	uuid_t	u;
	size_t 	size, bytea_len;
	size_t	sql_len = SIZE_SQL;
	ValueStruct *value;
	char	importtime[50];

	id = xmalloc(SIZE_TERM+1);
	uuid_generate(u);
	uuid_unparse(u, id);

	if ((value = file_to_bytea(dbg, filename)) == NULL){
		return NULL;
	}
	bytea = ValueToString(value,NULL);
	bytea_len = strlen(bytea);
	if (lifetype > 2) {
		lifetype = 2;
	}
	timestamp(importtime, sizeof(importtime));
	sql = xmalloc(bytea_len + sql_len);
	sql_p = sql;
	size = snprintf(sql_p, sql_len, \
		   "INSERT INTO monblob \
                        (id, importtime, lifetype, filename, file_data) \
                 VALUES ('%s', '%s', %d, '%s', '",id, importtime, lifetype, filename);
	sql_p = sql_p + size;
	strncpy(sql_p, bytea, bytea_len);
	sql_p = sql_p + bytea_len;
	snprintf(sql_p, sql_len, "');");
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	FreeValueStruct(value);
	xfree(sql);
	return id;
}

static	char *
blob_import(
	DBG_Struct	*dbg,
	char *filename,
	unsigned int lifetype)
{
	char *id;

	TransactionStart(dbg);
	id = file_import(dbg, filename, lifetype);
	TransactionEnd(dbg);

	return id;
}

static char *
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

static char *
file_export(
	DBG_Struct	*dbg,
	char *id,
	char *export_file)
{
	static char	*filename;
	char	*sql;
	size_t	sql_len = SIZE_SQL;
	ValueStruct	*ret, *value, *retval;

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT filename, file_data FROM monblob WHERE id = '%s'", id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (!ret) {
		fprintf(stderr,"ERROR: [%s] is not registered\n", id);
		return NULL;
	}
	if (export_file == NULL) {
		export_file = StrDup(ValueToString(GetItemLongName(ret,"filename"),dbg->coding));
	}
	value = GetItemLongName(ret,"file_data");
	retval = unescape_bytea(dbg, value);
	filename = value_to_file(export_file, retval);
	FreeValueStruct(retval);

	return filename;
}

static	char *
blob_export(
	DBG_Struct	*dbg,
	char *id,
	char *export_file)
{
	char *filename;

	TransactionStart(dbg);
	filename = file_export(dbg, id, export_file);
	TransactionEnd(dbg);

	return filename;
}

static char *
valstr(
	ValueStruct *value,
	char *name)
{
	return ValueToString(GetItemLongName(value, name),NULL);

}

static void
_list_print(
	ValueStruct *value)
{
	printf("%s\t%s\t%s\n",
		   valstr(value,"id"),
		   valstr(value,"importtime"),
		   valstr(value,"filename"));
}

static  void
list_print(
	ValueStruct *ret)
{
	int i;
	ValueStruct *value;

	if (ret == NULL) {
		return;
	}
	if (ValueType(ret) == GL_TYPE_ARRAY) {
		for	( i = 0 ; i < ValueArraySize(ret) ; i ++ ) {
			value = ValueArrayItem(ret,i);
			_list_print(value);
		}
	} else if (ValueType(ret) == GL_TYPE_RECORD) {
		_list_print(ret);
	}
}

static	void
blob_list(
	DBG_Struct	*dbg)
{
	char	*sql;
	size_t	sql_len = SIZE_SQL;
	ValueStruct *ret;

	TransactionStart(dbg);

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT importtime, id, filename FROM monblob ORDER BY importtime;");
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	list_print(ret);
	FreeValueStruct(ret);
	xfree(sql);
	TransactionEnd(dbg);
}

static	void
blob_delete(
	DBG_Struct	*dbg,
	char *id)
{
	char	*sql;
	size_t	sql_len = SIZE_SQL;

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "DELETE FROM monblob WHERE id = '%s'", id);
	TransactionStart(dbg);
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	TransactionEnd(dbg);
	xfree(sql);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	DBG_Struct	*dbg;
	char *id;
	char *filename;

	SetDefault();
	GetOption(option,argc,argv,NULL);
	InitSystem();

	if ( (ImportFile == NULL)
		 && (ExportID == NULL)
		 && (List == FALSE )
		 && (DeleteID == NULL) ) {
		PrintUsage(option,argv[0],NULL);
		exit(1);
	}

	dbg = GetDBG_monsys();
	dbg->dbt = NewNameHash();

	if (OpenDB(dbg) != MCP_OK ) {
		exit(1);
	}
	monblob_setup(dbg);

	if (List) {
		blob_list(dbg);
	} else if (ImportFile) {
		id = blob_import(dbg, ImportFile, LifeType);
		if ( !id ){
			exit(1);
		}
		printf("%s\n", id);
	} else if (ExportID) {
		filename = blob_export(dbg, ExportID, OutputFile);
		if ( !filename ) {
			exit(1);
		}
		printf("export: %s\n", filename);
	} else if (DeleteID){
		blob_delete(dbg, DeleteID);
	}
	CloseDB(dbg);

	return 0;
}
