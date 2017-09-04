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

static	char			*Directory;
static	char			*ImportFile;
static	Bool			List;
static	Bool			Blob;
static	char			*DeleteID;
static	char			*InfoID;
static	char			*ExportID;
static	char			*OutputFile;
static	unsigned int	LifeType;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"import",	STRING,		TRUE,	(void*)&ImportFile,
		"Import file name"								},
	{	"export",	STRING,		TRUE,	(void*)&ExportID,
		"Export ID"										},
	{	"blob",		BOOLEAN,	TRUE,	(void*)&Blob,
		"user BLOB ID instead of ID"					},
	{	"list",		BOOLEAN,	TRUE,	(void*)&List,
		"list of imported file"							},
	{	"delete",	STRING,		TRUE,	(void*)&DeleteID,
		"Delete imported file"							},
	{	"info",		STRING,		TRUE,	(void*)&InfoID,
		"display Info for ID"							},
	{	"output",	STRING,		TRUE,	(void*)&OutputFile,
		"output file name"								},
	{	"lifetype",	INTEGER,	TRUE,	(void*)&LifeType,
		"lifetime type"									},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory	= NULL;
	ImportFile	= NULL;
	ExportID	= NULL;
	OutputFile	= NULL;
	LifeType	= 1;
	List		= FALSE;
	Blob		= FALSE;
	fTimer		= TRUE;
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
	printf("%s\t%s\t%s\t%s\n",
		   valstr(value,"id"),
		   valstr(value,"blobid"),
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
			 "SELECT importtime, id, blobid, filename FROM %s ORDER BY importtime;", MONBLOB);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	list_print(ret);
	FreeValueStruct(ret);
	xfree(sql);
	TransactionEnd(dbg);
}

static	void
blob_info(
	DBG_Struct	*dbg,
	char *id)
{
	char	*sql;
	size_t	sql_len = SIZE_SQL;
	ValueStruct *ret;

	TransactionStart(dbg);

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT importtime, id, blobid, filename, size FROM %s WHERE id = '%s';", MONBLOB,id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	list_print(ret);
	FreeValueStruct(ret);
	xfree(sql);
	TransactionEnd(dbg);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	DBG_Struct	*dbg;
	char *id;
	Bool rc;
	MonObjectType oid;

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
		CloseDB(dbg);
		return 0;
	} 

	if (Blob) {
		if (ImportFile) {
			TransactionStart(dbg);
			oid = blob_import(dbg, 0, ImportFile, NULL, LifeType);
			TransactionEnd(dbg);
			printf("%d\n", (int)oid);
		} else if (ExportID) {
			TransactionStart(dbg);
			oid = (MonObjectType)atoi(ExportID);
			rc = blob_export(dbg, oid, OutputFile);
			TransactionEnd(dbg);
			if (!rc) {
				exit(1);
			}
			printf("export: %s\n", OutputFile);
		} else if (DeleteID){
			oid = (MonObjectType)atoi(DeleteID);
			blob_delete(dbg, oid);
		} else if (InfoID) {
			fprintf(stderr,"not implement\n");
			exit(1);
		}
	} else {
		if (ImportFile) {
			TransactionStart(dbg);
			id = monblob_import(dbg, NULL, 1, ImportFile, NULL, LifeType);
			TransactionEnd(dbg);
			if ( !id ){
				exit(1);
			}
			printf("%s\n", id);
			xfree(id);
		} else if (ExportID) {
			TransactionStart(dbg);
			rc = monblob_export(dbg, ExportID, OutputFile);
			TransactionEnd(dbg);
			if (!rc) {
				exit(1);
			}
			printf("export: %s\n", OutputFile);
		} else if (DeleteID){
			monblob_delete(dbg, DeleteID);
		} else if (InfoID) {
			blob_info(dbg,InfoID);
		}
	}
	CloseDB(dbg);

	return 0;
}
