/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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

#define	DEBUG
#define	TRACE
/*
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"defaults.h"
#include	"handler.h"		/*	for LibPath	*/
#include	"load.h"
#include	"loader.h"
#include	"debug.h"

static	char	*APS_LoadPath;

extern	void	*
LoadModule(
	GHashTable	*table,
	char	*name)
{
	char		funcname[SIZE_BUFF]
	,			filename[SIZE_BUFF];
	void		*f_main;
	void		(*f_init)(void);
	void		*handle;

dbgmsg(">LoadModule");
	if		(  ( f_main = (void *)g_hash_table_lookup(table,name) )  ==  NULL  ) {
		sprintf(filename,"%s.so",name);
printf("[%s]\n",filename);
		if		(  ( handle = LoadFile(APS_LoadPath,filename) )  !=  NULL  ) {
			sprintf(funcname,"%sInit",name);
printf("[%s]\n",funcname);
			if		(  ( f_init = (void *)dlsym(handle,funcname) )  !=  NULL  ) {
				f_init();
			}
			sprintf(funcname,"%sMain",name);
			f_main = dlsym(handle,funcname);
			g_hash_table_insert(table,StrDup(name),f_main);
		} else {
			fprintf(stderr,"[%s] not found.\n",name);
		}
	}
dbgmsg("<LoadModule");
	return	(f_main);
}

extern	GHashTable	*
InitLoader(void)
{
	GHashTable	*table;
	char		*path;

dbgmsg(">InitLoader");
	table = NewNameHash();
	if		(  LibPath  ==  NULL  ) { 
		if		(  ( path = getenv("APS_LOAD_PATH") )  ==  NULL  ) {
			APS_LoadPath = MONTSUQI_LOAD_PATH;
		} else {
			APS_LoadPath = path;
		}
	} else {
		APS_LoadPath = LibPath;
	}
dbgmsg("<InitLoader");
	return	(table);
}

