/*	OSEKI -- Object Store Engine Kernel Infrastructure

Copyright (C) 2004 Ogochan.

This module is part of OSEKI.

	OSEKI is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
OSEKI, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with OSEKI so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdlib.h>
#include	<glib.h>

#include	"types.h"
#include	"table.h"
#include	"debug.h"

static	guint
IdHash(
	uint64_t	*key)
{
	guint	ret;

	ret = (guint)*key;
	return	(ret);
}

static	gint
IdCompare(
	uint64_t	*o1,
	uint64_t	*o2)
{
	int		check;

	if		(	(  o1  !=  NULL  )
			&&	(  o2  !=  NULL  ) ) {
		check = *o1 - *o2;
	} else {
		check = 1;
	}
	return	(check == 0);
}

extern	GHashTable	*
NewLLHash(void)
{
	return	(g_hash_table_new((GHashFunc)IdHash,(GCompareFunc)IdCompare));
}

static	int
intcmp(
	int		o1,
	int		o2)
{
	return	(o1-o2);
}

extern	GTree	*
NewIntTree(void)
{
	return	(g_tree_new((GCompareFunc)intcmp));
}
