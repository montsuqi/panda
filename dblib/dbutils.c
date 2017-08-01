/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	"libmondai.h"
#include	"dblib.h"
#include	"dbgroup.h"

extern Bool
table_exist(
	DBG_Struct	*dbg,
	char *table_name)
{
	ValueStruct *ret;
	char *sql;
	sql = (char *)xmalloc(SIZE_SQL);
	Bool 	rc;

	snprintf(sql, SIZE_SQL, "SELECT 1 FROM  pg_catalog.pg_class c JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace WHERE c.relname = '%s' AND c.relkind = 'r';", table_name);
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

extern Bool
column_exist(
	DBG_Struct	*dbg,
	char *table_name,
	char *column_name)
{
	Bool rc;
	char *sql, *p;
	ValueStruct *ret;

	sql = (char *)xmalloc(SIZE_SQL);
	p = sql;
	p += sprintf(p, "SELECT 1");
	p += sprintf(p, " FROM pg_tables JOIN information_schema.columns on pg_tables.tablename = columns.table_name ");
	p += sprintf(p, " WHERE table_name = '%s' AND column_name = '%s'", table_name, column_name);
	sprintf(p, ";");
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

extern void
timestamp(
	char *daytime,
	size_t size)
{
	time_t now;
	struct	tm	tm_now;

	now = time(NULL);
	localtime_r(&now, &tm_now);
	strftime(daytime, size, "%F %T %z", &tm_now);
}
