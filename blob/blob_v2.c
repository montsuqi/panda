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

#define	DEBUG
#define	TRACE
/*
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

#define	ROUND_TO(p,s)	((((p)%(s)) == 0) ? (p) : (((p)/(s))+1)*(s))

static	void	WriteBuffer(BLOB_V2_State *state, BLOB_V2_Open *ent);

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
					nodepage = GetPage(state,page);
					lower = page;
					node = 0;
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
				leafpage[j].pos = 0;
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
printf("off = %d\n",off);
		if		(	(  leafnode[off].mode  ==  BLOB_OPEN_CLOSE  )
				||	(	(  leafnode[off].mode  ==  BLOB_OPEN_READ  )
					&&	(  mode                ==  BLOB_OPEN_READ  ) ) ) {
			if		(  ( mode & BLOB_OPEN_WRITE )  !=  0  ) {
				//				leafnode[off].size = obj;	/*	dummy	*/
			}
			leafnode[off].mode = mode;
printf("leafnode[off].pos = %lld\n",leafnode[off].pos);
			if		(	(  leafnode[off].pos            !=  0  )
					&&	(  ( mode & BLOB_OPEN_CREATE )  !=  0  ) ) {
				/*	remove	*/
				leafnode[off].pos = 0;
				leafnode[off].size = 0;
			}
			ent = New(BLOB_V2_Open);
			ent->obj = obj;
			ent->mode = mode;
			ent->head = leafnode[off].pos;
			ent->first = leafnode[off].pos;
printf("ent->head = %lld\n",ent->head);
			ent->last = ent->head;
			ent->size = leafnode[off].size;
printf("ent->size = %lld\n",ent->size);
			ent->buff = xmalloc(state->space->pagesize);
			ent->bsize = 0;
			ent->use = 0;
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
	WriteBuffer(state,ent);
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
			if		(  ( leafnode[off].mode & BLOB_OPEN_WRITE )  !=  0  ) {
				leafnode[off].size = ent->size;
				leafnode[off].pos = ent->first;
			}
			leafnode[off].mode = BLOB_OPEN_CLOSE;
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
		for	( i = 0 ; i < blob->level ; i ++ ) {
			page = stack[i];
			nodepage = GetPage(state,page);
			for	( j = 0 ; j < NODE_ELEMENTS(state) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					FREE_NODE(nodepage[j]);
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

static	pageno_t
GetFreePage(
	BLOB_V2_State	*state)
{
	BLOB_V2_Space	*blob;
	pageno_t		no;
	int				i;

ENTER_FUNC;
	blob = state->space;
	no = 0;
	for	( i = 0 ; i < blob->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  blob->freedata[i]  !=  0  ) {
			no = blob->freedata[i];
			blob->freedata[i] = 0;
			WritePage(state->space,state->space->freedata,
					  state->space->freedatapage);
			break;
		}
	}
	if		(  no  ==  0  ) {
		no = NewPage(state);
	}
LEAVE_FUNC;
	return	(no);
}

static	void
ReturnPage(
	BLOB_V2_State	*state,
	pageno_t		no)
{
	BLOB_V2_Space	*blob;
	int				i;

ENTER_FUNC;
	blob = state->space;
	for	( i = 0 ; i < blob->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  blob->freedata[i]  ==  0  ) {
			blob->freedata[i] = no;
			break;
		}
	}
	WritePage(state->space,state->space->freedata,state->space->freedatapage);
LEAVE_FUNC;
}

static	void
WriteBuffer(
	BLOB_V2_State	*state,
	BLOB_V2_Open	*ent)
{
	pageno_t			page
		,				wpage;
	BLOB_V2_DataPage	*data;
	BLOB_V2_DataEntry	*el
		,				*wel;
	objpos_t			pos;
	size_t				csize
		,				left;

ENTER_FUNC;
	if		(  ent->head  ==  0  ) {
		page = GetFreePage(state);
		data = GetPage(state,page);
		left = state->space->pagesize
			- ( data->use + sizeof(BLOB_V2_DataEntry) );
		if		(  left  <  ent->use  ) {
			csize = left;
		} else {
			csize = ent->use;
		}
printf("csize = %d\n",csize);
		if		(  csize  >  0  ) {
			el = (BLOB_V2_DataEntry *)((byte *)data + data->use);
			pos = page * state->space->pagesize + data->use;
			el->size = csize;
			el->next = 0;
			el->prio = ent->last;
			memcpy((byte *)el + sizeof(BLOB_V2_DataEntry),ent->buff,csize);
			memcpy(ent->buff,ent->buff+csize,csize);
			ent->use -= csize;
			data->use += ROUND_TO(csize,sizeof(size_t))
				+ sizeof(BLOB_V2_DataEntry);
			UpdatePage(state,page);
			if		(  data->use  <  state->space->pagesize  ) {
				ReturnPage(state,page);
			}
printf("pos = %lld\n",pos);
			if		(  ent->last  !=  ent->head  ) {
dbgmsg("*");
				wpage = ent->last / state->space->pagesize;
				data = GetPage(state,wpage);
				wel = (BLOB_V2_DataEntry *)
					((byte *)data + ent->last % state->space->pagesize);
				wel->next = pos;
				UpdatePage(state,wpage);
				ReleasePage(state,wpage);
			} else {
dbgmsg("*");
				ent->first = pos;
			}
			ent->last = pos;
		}
		ReleasePage(state,page);
	} else {
	}
LEAVE_FUNC;
}

extern	int
WriteBLOB_V2(
	BLOB_V2_State	*state,
	MonObjectType	obj,
	byte			*buff,
	size_t			size)
{
	BLOB_V2_Open	*ent;
	int			ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	LockBLOB(state->space);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = 0;
		while	(  size  >  0  ) {
			left = state->space->pagesize - ent->use;
			csize = ( size < left ) ? size : left;
			memcpy(&ent->buff[ent->use],buff,csize);
			ent->use += csize;
			ret += csize;
			size -= csize;
			buff += csize;
printf("use = %d\n",ent->use);
			if		(  ent->use  ==  state->space->pagesize  ) {
				WriteBuffer(state,ent);
			}
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
ReadBuffer(
	BLOB_V2_State	*state,
	BLOB_V2_Open	*ent)
{
	pageno_t			page;
	BLOB_V2_DataPage	*data;
	BLOB_V2_DataEntry	*el;
	size_t				off;

ENTER_FUNC;
	if		(  ent->last  >  0  ) {
		page = ent->last / state->space->pagesize;
		off = ent->last % state->space->pagesize;
printf("page = %lld\n",page);
printf("off = %d\n",off);
		data = GetPage(state,page);
		el = (BLOB_V2_DataEntry *)((byte *)data + off);
		memcpy(ent->buff,(byte *)el + sizeof(BLOB_V2_DataEntry),el->size);
		ent->bsize = el->size;
printf("el->size = %d\n",el->size);
		ent->last = el->next;
		ent->use = 0;
		ReleasePage(state,page);
	} else {
		ent->bsize = 0;
	}
LEAVE_FUNC;
}

extern	int
ReadBLOB_V2(
	BLOB_V2_State	*state,
	MonObjectType	obj,
	byte			*buff,
	size_t			size)
{
	BLOB_V2_Open	*ent;
	int			ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	LockBLOB(state->space);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = 0;
		while	(  size  >  0  ) {
			left = ent->bsize - ent->use;
			if		(  left  ==  0  ) {
				ReadBuffer(state,ent);
				if		(  ent->bsize  ==  0  )	break;
				left = ent->bsize;
			}
			csize = ( size < left ) ? size : left;
			memcpy(buff,&ent->buff[ent->use],csize);
			ent->use += csize;
			size -= csize;
			ret += csize;
			buff += csize;
		}
	} else {
		ret = -1;
	}
	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
LEAVE_FUNC;
	return	(ret);
}
