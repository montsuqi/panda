/*
 * OSEKI -- Object Store Engine Kernel Infrastructure
 * Copyright (C) 2004-2008 Ogochan.
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
