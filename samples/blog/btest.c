/*	Panda -- a simple transaction monitor

Copyright (C) 2003 Ogochan.

This module is part of Panda.

	Panda is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
Panda, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with Panda so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/
/*
*/
#define	DEBUG
#define	TRACE


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"apslib.h"
#include	"message.h"
#include	"debug.h"

#define	SRC_CODING	"euc-jp"

static	ValueStruct	*DB_BlogBody;

extern	int
btestMain(
	int		argc,
	char	**argv)
{
	int		rc;
	int		oid;
	char	buff[SIZE_LONGNAME+1];

ENTER_FUNC;
	DB_BlogBody = MCP_GetDB_Define("blog_body");
	rc = MCP_ExecFunction(NULL,NULL,NULL,"DBOPEN",NULL);
	rc = MCP_ExecFunction(NULL,NULL,NULL,"DBSTART",NULL);

	for	(  oid = 0 ; oid <  100 ; oid ++ ) {
		SetValueInteger(GetItemLongName(DB_BlogBody,"object"),oid);
		rc = MCP_ExecFunction(NULL,"blog_body","","BLOBCHECK",DB_BlogBody);
		printf("rc = [%d]\n",rc);
		if		(  rc  ==  0  ) {
			sprintf(buff,"./export_%d",oid);
			SetValueString(GetItemLongName(DB_BlogBody,"file"),buff,NULL);
			rc = MCP_ExecFunction(NULL,"blog_body","","BLOBEXPORT",DB_BlogBody);
		}
	}

	rc = MCP_ExecFunction(NULL,NULL,NULL,"DBCOMMIT",NULL);
	rc = MCP_ExecFunction(NULL,NULL,NULL,"DBDISCONNECT",NULL);

LEAVE_FUNC;
	return	(0);
}
