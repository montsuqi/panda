/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2004 Ogochan

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
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<errno.h>
#include	"types.h"
#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"
#include	"blob_v2.h"
#include	"message.h"
#include	"debug.h"

static	MonObjectType
GetFreeOID(
	BLOB_V2_State	*state)
{
	pageno_t	*nodepage;
	pageno_t	stack[MAX_PAGE_LEVEL];
	pageno_t	page
		,		node
		,		leaf
		,		lower
		,		own;
	int			i
		,		j
		,		k
		,		use
		,		off;
	MonObjectType	base;
	BLOB_V2_Space	*blob;
	BLOB_V2_Entry	*leafpage;
	Bool		fFree;

ENTER_FUNC;
	blob = state->space;
	leaf = 0;
	base = 0;
	while	(  leaf  ==  0  ) {
		for	( i = 0 ; i < blob->level ; i ++ ) {
			for	( j = 0 ; j < blob->level ; j ++ ) {
				stack[j] = blob->pos[j];
			}
			page = blob->pos[i];
			if		(  HAVE_FREECHILD(page)  ) {
				base = 0;
				for	( k = i ; k >= 0 ; k -- ) {
					stack[k] = page;
dbgprintf("k    = %d\n",k);
dbgprintf("page = %lld\n",page);
					nodepage = GetPage(state,page);
					lower = page;
					node = 0;
#if	0
for	( j = 0 ; j <  NODE_ELEMENTS(state) ; j ++ ) {
	printf("dump %lld\n",PAGE_NO(nodepage[j]));
}
#endif
					for	( j = 0 ; j <  NODE_ELEMENTS(state) ; j ++ ) {
						if		(  HAVE_FREECHILD(nodepage[j])  ) {
							if		(  ( lower = PAGE_NO(nodepage[j]) )  ==  0  ) {
								lower = NewPage(state);
								if		(  k  >  0  ) {
									nodepage[j] = lower | PAGE_NODE;
								} else {
									nodepage[j] = lower;
								}
								UpdatePage(state,page);
							}
							leaf = lower;
printf("new leaf = %lld\n",leaf);
							node = j;
							break;
						}
					}
					ReleasePage(state,page);
					page = lower;
					base = base + node * state->space->mul[k];
				}
				if		(  leaf  >  0  )	break;
			}
		}
		if		(  leaf  ==  0  ) {
dbgprintf("level up\n");
			node = NewPage(state);
			nodepage = GetPage(state,node);
			nodepage[0] = blob->pos[blob->level-1] | PAGE_NODE;
			blob->pos[blob->level] = node;
			stack[blob->level] = node;
			blob->level ++;

			UpdatePage(state,node);
			ReleasePage(state,node);
		}
	}
	leafpage = GetPage(state,leaf);
	off = -1;
	use = 0;
	for	( j = 0 ; j < LEAF_ELEMENTS(state) ; j ++ ) {
		if		(  IS_FREEOBJ(leafpage[j])  )	{
			if		(  off  <  0  ) {
				USE_OBJ(leafpage[j]);
				leafpage[j].mode = BLOB_OPEN_CLOSE;
				off = j;
				use ++;
			}
		} else {
			use ++;
		}
	}
	if		(  off  <  0  ) {
		fprintf(stderr,"tree structure broken.\n");
		exit(1);
	}
	UpdatePage(state,leaf);
	ReleasePage(state,leaf);
	if		(  use  ==  LEAF_ELEMENTS(state)  ) {
#ifdef	DEBUG
		dbgprintf("stack dump\n");
		for	( i = 0 ; i < blob->level ; i ++ ) {
			dbgprintf("stack[%d] = %lld\n",i,stack[i]);
		}
#endif
		own = leaf;
		use = 0;
		for	( i = 0 ; i < blob->level ; i ++ ) {
			page = stack[i];
			nodepage = GetPage(state,page);
			fFree = FALSE;
			for	( j = 0 ; j < NODE_ELEMENTS(state) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					USE_NODE(nodepage[j]);
				}
				if		(  HAVE_FREECHILD(nodepage[j])  ) {
					fFree = TRUE;
					break;
				}
			}
			own = page;
			UpdatePage(state,page);
			ReleasePage(state,page);
			if		(  fFree  )	break;
			if		(  PAGE_NO(blob->pos[i])  ==  page  ) {
				USE_NODE(blob->pos[i]);
			}
		}
	}
LEAVE_FUNC;
	return	(base * LEAF_ELEMENTS(state) + off);
}

static	pageno_t
SearchLeafPage(
	BLOB_V2_State	*state,
	MonObjectType	obj)
{
	pageno_t	off
		,		base
		,		page
		,		next
		,		*nodepage;
	BLOB_V2_Space	*blob;
	int			i;

ENTER_FUNC;
	blob = state->space;
	base = obj / LEAF_ELEMENTS(state);
	page = 0;
	for	( i = blob->level - 1 ; i >= 0 ; i -- ) {
		off = base / blob->mul[i];
		if		(  page  ==  0  ) {
			page = blob->pos[i];
		}
		nodepage = GetPage(state,page);
		next = PAGE_NO(nodepage[off]);
		ReleasePage(state,page);
		page = next;
		base %= NODE_ELEMENTS(state);
	}
LEAVE_FUNC;
	return	(page);
}

static	BLOB_V2_Open	*
OpenEntry(
	BLOB_V2_State	*state,
	pageno_t		leaf,
	MonObjectType	obj,
	byte			mode)
{
	BLOB_V2_Entry	*leafnode;
	int		off;
	BLOB_V2_Open	*ent;

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  ==  NULL  ) {
		leafnode = (BLOB_V2_Entry *)GetPage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		if		(	(  leafnode[off].mode  ==  BLOB_OPEN_CLOSE  )
				||	(	(  leafnode[off].mode  ==  BLOB_OPEN_READ  )
					&&	(  mode                ==  BLOB_OPEN_READ  ) ) ) {
			if		(  ( mode & BLOB_OPEN_WRITE )  !=  0  ) {
				leafnode[off].size = obj;	/*	dummy	*/
			}
			leafnode[off].mode = mode;
			ent = New(BLOB_V2_Open);
			ent->obj = obj;
			ent->mode = mode;
			ent->head = leafnode[off].pos;
			ent->last = leafnode[off].pos;
			ent->size = leafnode[off].size;
			ent->buff = xmalloc(state->space->pagesize);
			ent->bleft = state->space->pagesize;
			g_hash_table_insert(state->oTable,&ent->obj,ent);
			UpdatePage(state,leaf);
		} else {
			ent = NULL;
		}
		ReleasePage(state,leaf);
	}
LEAVE_FUNC;
	return	(ent);
}

extern	MonObjectType
NewBLOB_V2(
	BLOB_V2_State	*state,
	int				mode)
{
	MonObjectType	obj;
	pageno_t		leaf;

ENTER_FUNC;
	LockBLOB(state->space);
	obj = GetFreeOID(state);
	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
	if		(  ( leaf = SearchLeafPage(state,obj) )  !=  0  ) {
		if		(  OpenEntry(state,leaf,obj,(byte)mode)  ==  NULL  ) {
			obj = GL_OBJ_NULL;
		}
	}
LEAVE_FUNC;
	return	(obj);
}

extern	ssize_t
OpenBLOB_V2(
	BLOB_V2_State	*state,
	MonObjectType	obj,
	int				mode)
{
	pageno_t	leaf;
	ssize_t		ret;
	BLOB_V2_Open	*ent;

ENTER_FUNC;
	LockBLOB(state->space);
	if		(  ( leaf = SearchLeafPage(state,obj) )  !=  0  ) {
		if		(  ( ent = OpenEntry(state,leaf,obj,(byte)mode) )  !=  NULL  ) {
			ret = -1;
		} else {
			ret = ent->size;
		}
	} else {
		ret = -1;
	}
	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
LEAVE_FUNC;
	return	(ret);
}

static	void
FlushBLOB_V2(
	BLOB_V2_State	*state,
	BLOB_V2_Open	*ent)
{
}

extern	Bool
CloseBLOB_V2(
	BLOB_V2_State	*state,
	MonObjectType	obj)
{
	pageno_t		leaf;
	BLOB_V2_Entry	*leafnode;
	Bool			ret;
	int				off;
	BLOB_V2_Open	*ent;

ENTER_FUNC;
	ret = TRUE;
	LockBLOB(state->space);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  !=  NULL  ) {
		FlushBLOB_V2(state,ent);
		if		(  ( leaf = SearchLeafPage(state,obj) )  !=  0  ) {
			leafnode = (BLOB_V2_Entry *)GetPage(state,leaf);
			off = obj % LEAF_ELEMENTS(state);
			leafnode[off].mode = BLOB_OPEN_CLOSE;
			leafnode[off].size = ent->size;
			UpdatePage(state,leaf);
			ReleasePage(state,leaf);
		}
		xfree(ent->buff);
		g_hash_table_remove(state->oTable,&obj);
		xfree(ent);
	}
	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
DestroyBLOB_V2(
	BLOB_V2_State	*state,
	MonObjectType	obj)
{
	int				i
		,			j
		,			off;
	BLOB_V2_Entry	*leafpage;
	pageno_t		*nodepage;
	Bool			rc;
	pageno_t		stack[MAX_PAGE_LEVEL];
	pageno_t		page
		,			base
		,			own
		,			next;
	BLOB_V2_Space	*blob;

ENTER_FUNC;
	rc = TRUE;
	LockBLOB(state->space);

	blob = state->space;
	base = obj / LEAF_ELEMENTS(state);
	page = 0;
	for	( i = blob->level - 1 ; i >= 0 ; i -- ) {
		off = base / blob->mul[i];
		if		(  page  ==  0  ) {
			page = blob->pos[i];
		}
		stack[i] = page;
		nodepage = GetPage(state,page);
		next = PAGE_NO(nodepage[off]);
		ReleasePage(state,page);
		page = next;
		base %= NODE_ELEMENTS(state);
	}

	if		(  page  ==  0  ) {
		rc = FALSE;
	} else {
		leafpage = GetPage(state,page);
		FREE_OBJ(leafpage[obj % LEAF_ELEMENTS(state)]);
		UpdatePage(state,page);
		ReleasePage(state,page);

		own = page;
printf("own = %lld\n",own);
		for	( i = 0 ; i < blob->level ; i ++ ) {
			page = stack[i];
			nodepage = GetPage(state,page);
			for	( j = 0 ; j < NODE_ELEMENTS(state) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					FREE_NODE(nodepage[j]);
printf("free page = %lld, off = %d\n",page,j);
					break;
				}
			}
			own = page;
			UpdatePage(state,page);
			ReleasePage(state,page);
			if			(  PAGE_NO(blob->pos[i])  ==  page  ) {
				FREE_NODE(blob->pos[i]);
			}
			if		(  HAVE_FREECHILD(page)  )	break;
		}
		rc = TRUE;
	}

	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
LEAVE_FUNC;
	return	(rc);
}
#if	0
extern	int
WriteBLOB_V2(
	BLOB_V2_Space		*blob,
	MonObjectType	obj,
	byte			*buff,
	size_t			size)
{
	pageno_t	leaf;

	BLOB_V2_Entry	*ent;
	int			ret;

ENTER_FUNC;
	LockBLOB(state->space);
	if		(  ( ent = g_hash_table_lookup(blob->table,(gpointer)&obj) )  !=  NULL  ) {
		if		(  ent->fp  !=  NULL  ) {
			ret = Send(ent->fp,buff,size);
		} else {
			ret = -1;
		}
	} else {
		ret = -1;
	}
	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
LEAVE_FUNC;
	return	(ret);
}

extern	int
ReadBLOB_V2(
	BLOB_V2_Space		*blob,
	MonObjectType	obj,
	byte			*buff,
	size_t			size)
{
	BLOB_V2_Entry	*ent;
	int			ret;

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(blob->table,(gpointer)&obj) )  !=  NULL  ) {
		if		(  ent->fp  !=  NULL  ) {
			ret = Recv(ent->fp,buff,size);
		} else {
			ret = -1;
		}
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}
#endif
