/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"oseki.h"
#include	"pagestruct.h"
#include	"oseki.h"
#include	"message.h"
#include	"debug.h"
#include	"option.h"

static	int		PageSize;

static	ARG_TABLE	option[] = {
	{	"pagesize",	INTEGER,	TRUE,	(void*)&PageSize,
		"入出力のページサイズ"							},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PageSize = 8192;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	FILE	*fp;
	pageno_t	*page;
	char	name[SIZE_LONGNAME+1];
	int		i;
	OsekiHeaderPage		head;
	OsekiObjectTable	table;
	OsekiObjectEntry	*ent;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("createpnb",NULL);
	if		(  PageSize  <  MIN_PAGE_SIZE  ) {
		fprintf(stderr,"page size too small, please set >= %d\n",MIN_PAGE_SIZE);
		exit(1);
	} else {
		page = (pageno_t *)xmalloc(PageSize);
	}
	if		(	(  fl  ==  NULL  )
			||	(  fl->name  ==  NULL  ) ) {
		fprintf(stderr,"must set space name.\n");
		exit(1);
	} else {
		snprintf(name,SIZE_LONGNAME+1,"%s.pnb",fl->name);
	}
	if		(  ( fp = Fopen(name,"w") )  ==  NULL  ) {
		fprintf(stderr,"can not open BLOB space.\n");
		exit(1);
	}

	memcpy(head.magic,OSEKI_FILE_HEADER,OSEKI_MAGIC_SIZE);
	head.pagesize = PageSize;

	/*	directory page	(0)	*/
	head.phase[0].pages = 5;
	head.phase[0].cOld = 0;
	head.phase[0].seq = 1;
	head.phase[1].pages = 5;
	head.phase[1].cOld = 0;
	head.phase[1].seq = 0;
	memclear(page,PageSize);
	memcpy(page,&head,sizeof(OsekiHeaderPage));
	fwrite(page,PageSize,1,fp);

	/*	object page		(1)	*/
	table.freedata = 2;
	table.level = 1;
	for	( i = 0 ; i < sizeof(pageno_t) ; i ++ ) {
		table.pos[i] = 0;
	}
	table.pos[0] = 3;
	memclear(page,PageSize);
	memcpy(page,&table,sizeof(OsekiObjectTable));
	fwrite(page,PageSize,1,fp);

	/*	free pages		(2)	*/
	memclear(page,PageSize);
	fwrite(page,PageSize,1,fp);

	/*	1st node page	(3)	*/
	memclear(page,PageSize);
	page[0] = 4;
	fwrite(page,PageSize,1,fp);

	/*	leaf page		(4)	*/
	memclear(page,PageSize);
	ent = (OsekiObjectEntry *)page;
	USE_OBJ(ent[0]);
	fwrite(page,PageSize,1,fp);

	fclose(fp);

	return	(0);
}
