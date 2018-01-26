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
#include	"monpushevent.h"
#include	"message.h"
#include	"debug.h"

static void
reset_id(
	DBG_Struct	*dbg)
{
	char *sql;
	ValueStruct *ret;

	sql = "SELECT setval('" SEQMONPUSHEVENT  "', 1, false);";
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	FreeValueStruct(ret);
}

extern int
new_monpushevent_id(
	DBG_Struct	*dbg)
{
	MonObjectType blobid;
	ValueStruct	*ret, *val;
	char *sql;

	sql = "SELECT nextval('" SEQMONPUSHEVENT  "') AS id;";
	ret = ExecDBQuery(dbg, sql, FALSE, DB_UPDATE);
	if (ret) {
		val = GetItemLongName(ret,"id");
		id = ValueToInteger(val);
		FreeValueStruct(ret);
	} else {
		id = 0;
	}
	if ((unsigned int)id >= USHRT_MAX) {
		reset_id(dbg);
	}
	return id;
}

static Bool
create_monpushevent(
	DBG_Struct *dbg)
{
	Bool rc;
	char *sql;	

	sql = ""
	"CREATE TABLE " MONPUSHEVENT " {"
	"  id        int,"
	"  event     varchar(128),"
	"  user_     varchar(128),"
	"  pushed_at timestamp with time_zone,"
	"  data	     varchar(2048)"
	"};";
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	if (rc != MCP_OK) {
		Warning("SQL Error:%s",sql);
		return FALSE;
	}
	sql = "CREATE INDEX " MONPUSHEVENT "_event ON " MONPUSHEVENT " (event);";
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	if (rc != MCP_OK) {
		Warning("SQL Error:%s",sql);
		return FALSE;
	}
	sql = "CREATE INDEX " MONPUSHEVENT "_user ON " MONPUSHEVENT " (user_);";
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	if (rc != MCP_OK) {
		Warning("SQL Error:%s",sql);
		return FALSE;
	}
	sql = "CREATE INDEX " MONPUSHEVENT "_pushed_at ON " MONPUSHEVENT " (pushed_at);";
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	if (rc != MCP_OK) {
		Warning("SQL Error:%s",sql);
		return FALSE;
	}

	sql = "CREATE SEQUENCE " SEQMONPUSHEVENT " ;";
	rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
	if (rc != MCP_OK) {
		Warning("SQL Error:%s",sql);
		return FALSE;
	}

	return TRUE;
}

extern Bool
monpushevent_setup(
	DBG_Struct	*dbg)
{
	int	rc;

	TransactionStart(dbg);
	if ( table_exist(dbg, MONPUSHEVENT) != TRUE) {
		create_monpushevent(dbg);
	}
	rc = TransactionEnd(dbg);
	return (rc == MCP_OK);
}

static	Bool
monpushevent_expire(
	DBG_Struct *dbg)
{
	Bool rc;
	char *sql;

	sql = "DELETE FROM " MONPUSHEVENT " WHERE (now() > pushed_at + CAST('" MONPUSHEVENT_EXPIRE " days' AS INTERVAL));";
	rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	return (rc == MCP_OK);
}

extern Bool
monpushevent_insert(
	DBG_Struct *dbg,
	int id,
	const char *user,
	const char *event,
	const char *pushed_at,
	const char *data)
{
	char *sql,*e_user,*e_event,*e_pushed_at,*e_data;
	int rc;

	monpushevent_expire(dbg);

	e_user 		= Escape_monsys(dbg,user);
	e_event 	= Escape_monsys(dbg,event);
	e_pushed_at	= Escape_monsys(dbg,pushed_at);
	e_data		= Escape_monsys(dbg,data);

	sql = g_strdup_printf("INSERT INTO %s (id,event,user_,pushed_at,data) VALUES ('%d','%s','%s','%s','%s');",MONPUSHEVENT,id,e_user,e_event,e_pushed_at,e_data);
	rc = ExecDBOP(dbg, sql, FALSE, DB_UPDATE);

	xfree(e_user);
	xfree(e_event);
	xfree(e_pushed_at);
	xfree(e_data);
	g_free(sql);

	return (rc == MCP_OK);
}
