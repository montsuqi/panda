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
#include	"comm.h"
#include	"gettext.h"
#include	"message.h"
#include	"debug.h"

#define RETRY 1

static	char	*Directory;
static	char	*ImportFile;
static	char	*ImportID;
static	char	*ExportID;
static	char	*Socket;
static	Bool	fSetup;

static	char	*OutputFile;
static	unsigned int	LifeType;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"setup",	BOOLEAN,	FALSE,	(void*)&fSetup,
		N_("table create")								},
	{	"socket",	STRING,		TRUE,	(void*)&Socket,
		"Socket file"									},
	{	"import",	STRING,		TRUE,	(void*)&ImportFile,
		"Import file name"								},
	{	"importid",	STRING,		TRUE,	(void*)&ImportID,
		"Import ID"										},
	{	"export",	STRING,		TRUE,	(void*)&ExportID,
		"Export ID"										},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory = NULL;
	fSetup = FALSE;
	ImportFile = NULL;
	ExportID = NULL;
	OutputFile = NULL;
	LifeType = 0;
}

static	void
InitSystem(void)
{
	char *dir;
	InitMessage("monblobapi",NULL);
	if ( (dir = getenv("MON_DIRECTORY_PATH")) != NULL ) {
		Directory = dir;
	}
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,P_NONE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
}

static int
SendValue(
	NETFILE *fp,
	ValueStruct	*value)
{
	int ret = 0;
	size_t size;

	if (value == NULL) {
		SendLength(fp,0);
	} else {
		size = ValueByteLength(value);
		SendLength(fp,size);
		ret = Send(fp,ValueByte(value),size);
	}
	return ret;
}

static NETFILE *
ConnectBlobAPI(
	char *socketfile)
{
	NETFILE *fp;
	Port	*port;
	int		fd;

	fp = NULL;
	port = ParPort(socketfile,NULL);
	fd = ConnectSocket(port, SOCK_STREAM);
	DestroyPort(port);
	if (fd > 0) {
		fp = SocketToNet(fd);
	} else {
		Error("cannot connect blob api");
	}
	return fp;
}

static void
DisconnectBlobAPI(NETFILE *fp)
{
	if (fp != NULL && CheckNetFile(fp)) {
		CloseNet(fp);
	}
}

static         Bool
create_monblobapi(
        DBG_Struct      *dbg)
{
	Bool rc;
	char *sql, *p;
	sql = (char *)xmalloc(SIZE_BUFF);
	p = sql;
	p += sprintf(p, "CREATE TABLE monblobapi (");
	p += sprintf(p, "       id        varchar(37)  primary key,");
	p += sprintf(p, "       importtime  timestamp  with time zone,");
	p += sprintf(p, "       lifetype    int,");
	p += sprintf(p, "       filename    varchar(4096),");
	p += sprintf(p, "       content_type varchar(1024),");
	p += sprintf(p, "       status		int,");
	p += sprintf(p, "       file_data   bytea");
	p += sprintf(p, ");");
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	xfree(sql);
	return rc;
}

extern Bool
monblobapi_setup(
	DBG_Struct	*dbg)
{
	Bool	exist;
	int 	rc;
	TransactionStart(dbg);
	exist = table_exist(dbg, "monblobapi");
	if ( !exist ) {
		create_monblobapi(dbg);
	}
	rc = TransactionEnd(dbg);
	return (rc == MCP_OK);
}

static char *
file_import(
	DBG_Struct	*dbg,
	char *id,
	char *filename,
	NETFILE *fp)
{
	char	*sql, *sql_p;
	char	*bytea;
	int lifetype;
	size_t 	size, bytea_len;;
	size_t	sql_len = SIZE_SQL;
	ValueStruct	*ret;
	ValueStruct *value;
	char	importtime[50];
	char *recv;
	char *regfilename;
	char *content_type;
	json_object *json_res;

	recv = RecvStringNew(fp);
	json_res = json_tokener_parse(recv);
	xfree(recv);

	json_object *json_id;
	json_object_object_get_ex(json_res,"id", &json_id);
	if (CheckJSONObject(json_id,json_type_string)) {
		id = (char*)json_object_get_string(json_id);
		sql = (char *)xmalloc(sql_len);
		snprintf(sql, sql_len,
				 "SELECT id FROM monblobapi WHERE id = '%s'", id);
		ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
		xfree(sql);
		if (!ret) {
			fprintf(stderr,"[%s] is not registered\n", id);
			SendString(fp, "NG");
			return NULL;
		}
	} else {
		if (id == NULL) {
			id = new_blobid();
			sql = xmalloc(sql_len);
			snprintf(sql, sql_len, "INSERT INTO monblobapi (id, status) VALUES('%s', '%d');", id , 503);
			ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
			xfree(sql);
		}
	}
	SendString(fp, id);
	if ((value = file_to_bytea(dbg, filename)) == NULL){
		return NULL;
	}
	bytea = ValueToString(value,NULL);
	bytea_len = strlen(bytea);

	json_object *json_content_type;
	json_object_object_get_ex(json_res,"content_type", &json_content_type);
	if (CheckJSONObject(json_content_type,json_type_string)) {
		content_type = (char*)json_object_get_string(json_content_type);
	} else {
		content_type = "application/octet-stream";
	}

	json_object *json_filename;
	json_object_object_get_ex(json_res,"filename", &json_filename);
	if (CheckJSONObject(json_filename,json_type_string)) {
		regfilename = (char*)json_object_get_string(json_filename);
	} else {
		regfilename = filename;
	}

	lifetype = 2;
	timestamp(importtime, sizeof(importtime));
	sql = xmalloc(bytea_len + sql_len);
	sql_p = sql;
	size = snprintf(sql_p, sql_len, \
					"UPDATE monblobapi SET importtime = '%s', lifetype = '%d', filename = '%s', content_type = '%s', status= '%d', file_data ='", \
					importtime, lifetype, regfilename, content_type, 200);
	sql_p = sql_p + size;
	strncpy(sql_p, bytea, bytea_len);
	sql_p = sql_p + bytea_len;
	snprintf(sql_p, sql_len, "' WHERE id='%s';", id);
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	FreeValueStruct(value);
	xfree(sql);
	return id;
}

static char *
file_export(
	DBG_Struct	*dbg,
	char *id,
	char *socket)
{
	static char	*filename;
	char	*sql;
	char	*str;
	size_t	sql_len = SIZE_SQL;
	json_object *obj;
	NETFILE *fp;
	ValueStruct	*ret, *value;
	ValueStruct	*retval;

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
			 "SELECT id, filename, content_type, status, file_data FROM monblobapi WHERE id = '%s'", id);
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);

	obj = json_object_new_object();
	if (!ret) {
		fprintf(stderr,"[%s] is not registered\n", id);
		json_object_object_add(obj,"status",json_object_new_int(404));
		retval = NULL;
	} else {
		value = GetItemLongName(ret,"file_data");
		retval = unescape_bytea(dbg, value);
		char *id;
		id = ValueToString(GetItemLongName(ret,"id"),dbg->coding);
		json_object_object_add(obj,"id",json_object_new_string(id));
		filename = ValueToString(GetItemLongName(ret,"filename"), dbg->coding);
		json_object_object_add(obj,"filename",json_object_new_string(filename));
		char *content_type;
		content_type = ValueToString(GetItemLongName(ret,"content_type"),dbg->coding);
		if (content_type == NULL || strlen(content_type) == 0 ) {
			json_object_object_add(obj,"content-type",json_object_new_string("application/octet-stream"));
		} else {
			json_object_object_add(obj,"content-type",json_object_new_string(content_type));
		}
		json_object_object_add(obj,"status",json_object_new_int(200));
	}
	str = (char*)json_object_to_json_string(obj);
	fp = ConnectBlobAPI(socket);
	SendString(fp, str);
	SendValue(fp, retval);
	FreeValueStruct(retval);
	Flush(fp);
	DisconnectBlobAPI(fp);
	FreeValueStruct(ret);
	return filename;
}

static	char *
blob_import(
	DBG_Struct	*dbg,
	char *id,
	char *filename,
	char *socket)
{
	NETFILE *fp;

	fp = ConnectBlobAPI(socket);

	TransactionStart(dbg);
	id = file_import(dbg, id, filename, fp);
	TransactionEnd(dbg);
	DisconnectBlobAPI(fp);

	return id;
}

static	char *
blob_export(
	DBG_Struct	*dbg,
	char *id,
	char *socket)
{
	char *filename;

	TransactionStart(dbg);
	filename = file_export(dbg, id, socket);
	TransactionEnd(dbg);

	return filename;
}

void
setup_only()
{
	DBG_Struct	*dbg;
	dbg = GetDBG_monsys();
	dbg->dbt = NewNameHash();
	if (OpenDB(dbg) != MCP_OK ) {
		exit(1);
	}
	monblobapi_setup(dbg);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	struct stat sb;
	DBG_Struct	*dbg;
	int i;
	SetDefault();
	GetOption(option,argc,argv,NULL);
	InitSystem();

	if (fSetup) {
		setup_only();
		exit(0);
	}
	if ((Socket == NULL) || ((ExportID == NULL) && (ImportFile == NULL)) ) {
		PrintUsage(option,argv[0],NULL);
		exit(1);
	}

	for (i=0; ; i++) {
		if (stat(Socket, &sb) != -1) {
			break;
		}
		if ( i >= RETRY ) {
			Error("%s %s", strerror(errno), Socket);
		}
		sleep(1);
	}
	if ( !S_ISSOCK(sb.st_mode) ) {
		Error("%s is not socket file", Socket);
	}

	dbg = GetDBG_monsys();
	dbg->dbt = NewNameHash();

	if (OpenDB(dbg) != MCP_OK ) {
		exit(1);
	}
	monblobapi_setup(dbg);
	if (ImportFile) {
		blob_import(dbg, ImportID, ImportFile, Socket);
	} else {
		blob_export(dbg, ExportID, Socket);
	}
	CloseDB(dbg);

	return 0;
}
