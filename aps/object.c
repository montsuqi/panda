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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"

#include	"libmondai.h"
#include	"object.h"
#include	"debug.h"

static	int		Oid;
static	GHashTable	*Object;

extern	int
NewObject(void)
{
	Element	*el;

	Oid ++;
	el = New(Element);
	el->oid = Oid;
	el->name = NULL;
	el->fp = NULL;

	return	(Oid);
}

extern	void
InitObject(void)
{
	Oid = 0;
	Object = NewIntHash();
}

