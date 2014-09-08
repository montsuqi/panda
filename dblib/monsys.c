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
#include	"directory.h"
#include	"dbgroup.h"
#include	"monsys.h"

extern DBG_Struct *
GetDBG_monsys(void)
{
	int i;
	DBG_Struct	*dbg = NULL;

	if (ThisEnv) {
		for ( i = 0; i < ThisEnv->cDBG; i++ ) {
			dbg = ThisEnv->DBG[i];
			if (!(strcmp(dbg->name, "")) && (!strcmp(dbg->type, "PostgreSQL"))) {
				break;
			}
		}
	}
	return dbg;
}

extern char *
Escape_monsys(
	DBG_Struct	*dbg,
	char *src)
{
	char *dest;
	ValueStruct	*value, *ret, *recval, *retval;
	DBCOMM_CTRL		*ctrl = NULL;
	RecordStruct	*rec = NULL;

	value = NewValue(GL_TYPE_CHAR);
	SetValueStringWithLength(value, src, strlen(src), NULL);

	recval = NewValue(GL_TYPE_RECORD);
	ValueAddRecordItem(recval, "dbescapestring", value);

	retval = ExecDBESCAPE(dbg, ctrl, rec, recval);
	if ( (ret = GetItemLongName(retval,"dbescapestring")) != NULL) {
		dest = StrDup(ValueToString(ret, dbg->coding));
	}
	FreeValueStruct(retval);
	FreeValueStruct(recval);
	return dest;
}
