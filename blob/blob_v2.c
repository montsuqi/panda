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
	MonObjectType	obj,
	pageno_t		*path)
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
		if		(  path  !=  NULL  ) {
			path[i] = page;
		}
		nodepage = GetPage(state,page);
		next = PAGE_NO(nodepage[off]);
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
		if		(  IS_FREEOBJ(leafnode[off])  ) {
			ent = NULL;
		} else
		if		(	(  leafnode[off].mode  ==  BLOB_OPEN_CLOSE  )
				||	(	(  leafnode[off].mode  ==  BLOB_OPEN_READ  )
					&&	(  mode                ==  BLOB_OPEN_READ  ) ) ) {
			if		(  ( mode & BLOB_OPEN_WRITE )  !=  0  ) {
				//leafnode[off].size = obj;	/*	dummy	*/
			}
			leafnode[off].mode = mode;
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
			ent->last = ent->head;
			ent->size = leafnode[off].size;
			ent->buff = xmalloc(state->space->pagesize);
			ent->bsize = 0;
			ent->use = 0;
			ent->node = OBJ_POS(state,leaf,off);
			g_hash_table_insert(state->oTable,&ent->obj,ent);
			UpdatePage(state,leaf);
		} else {
			ent = NULL;
		}
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
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  OpenEntry(state,leaf,obj,(byte)mode|BLOB_OPEN_CREATE)
				   ==  NULL  ) {
			obj = GL_OBJ_NULL;
		}
	} else {
		obj = GL_OBJ_NULL;
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
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  ( ent = OpenEntry(state,leaf,obj,(byte)mode) )
				   ==  NULL  ) {
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
	if		(  ( ent->mode & BLOB_OPEN_WRITE )  !=  0  ) {
		while	(  ent->use  >  0  ) {
			WriteBuffer(state,ent);
		}
	}
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
	dbgprintf("obj = %lld\n",obj);
	ret = TRUE;
	LockBLOB(state->space);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  !=  NULL  ) {
		FlushBLOB_V2(state,ent);
		if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
			leafnode = (BLOB_V2_Entry *)GetPage(state,leaf);
			off = obj % LEAF_ELEMENTS(state);
			if		(  ( leafnode[off].mode & BLOB_OPEN_WRITE )  !=  0  ) {
				leafnode[off].size = ent->size;
				leafnode[off].pos = ent->first;
			}
			leafnode[off].mode = BLOB_OPEN_CLOSE;
			UpdatePage(state,leaf);
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

static	void
FreeEntry(
	BLOB_V2_State	*state,
	BLOB_V2_Entry	*ent)
{
	objpos_t	pos
		,		npos;
	pageno_t	page
		,		ppage
		,		npage
		,		leaf;
	size_t		off
		,		cur
		,		nsize
		,		shlink;
	BLOB_V2_DataPage	*data
		,				*wd;
	BLOB_V2_DataEntry	*el
		,				*wel;
	BLOB_V2_Entry		*leafnode;

ENTER_FUNC;
	pos = ent->pos;
	while	(  pos  !=  0  ) {
		page = OBJ_PAGE(state,pos);
		data = GetPage(state,page);
		off = OBJ_OFF(state,pos);
		shlink = 0;
		for	( cur = sizeof(BLOB_V2_DataPage) ; cur < data->use ;  ) {
			el = (BLOB_V2_DataEntry *)((byte *)data + cur);
			nsize = ROUND_TO(el->size,sizeof(size_t))
				+ sizeof(BLOB_V2_DataEntry);
			if		(  cur  ==  off  ) {
				pos = el->next;
				memmove((byte *)data + cur,
					   (byte *)data + cur + nsize,
						data->use - cur - nsize);
				data->use -= nsize;
				off = 0;
				el = (BLOB_V2_DataEntry *)((byte *)data + cur);
				nsize = ROUND_TO(el->size,sizeof(size_t))
					+ sizeof(BLOB_V2_DataEntry);
			}
			if		(  cur  >  off  ) {
				npos = OBJ_POS(state,page,cur);
				if		(  !IS_OBJ_NODE(el->prio)  ) {
					ppage = OBJ_PAGE(state,el->prio);
					wd = GetPage(state,ppage);
					wel = (BLOB_V2_DataEntry *)((byte *)wd
												+ sizeof(BLOB_V2_DataPage)
												+ OBJ_OFF(state,el->prio));
					wel->next = npos;
					UpdatePage(state,ppage);
				} else {
					leaf = OBJ_PAGE(state,el->prio);
					leafnode = (BLOB_V2_Entry *)GetPage(state,leaf);
					leafnode[OBJ_OFF(state,el->prio)].pos = npos;
					UpdatePage(state,leaf);
				}
				if		(  el->next  >  0  ) {
					npage = OBJ_PAGE(state,el->next);
					wd = GetPage(state,npage);
					wel = (BLOB_V2_DataEntry *)((byte *)wd
												+ sizeof(BLOB_V2_DataPage)
												+ OBJ_OFF(state,el->next));
					wel->prio = npos;
					UpdatePage(state,npage);
				}
			} else {
			}
			cur += nsize;
		}
		if		(  data->use  <  PAGE_SIZE(state)  ) {
			ReturnPage(state,page);
		}
		UpdatePage(state,page);
	}
LEAVE_FUNC;
}
	

extern	Bool
DestroyBLOB_V2(
	BLOB_V2_State	*state,
	MonObjectType	obj)
{
	int				i
		,			j;
	BLOB_V2_Entry	*leafpage;
	pageno_t		*nodepage;
	Bool			rc;
	pageno_t		stack[MAX_PAGE_LEVEL];
	pageno_t		page
		,			own;
	BLOB_V2_Space	*blob;
	size_t			off;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	rc = TRUE;
	LockBLOB(state->space);
	page = SearchLeafPage(state,obj,stack);
	if		(  page  ==  0  ) {
		rc = FALSE;
	} else {
		leafpage = GetPage(state,page);
		off = obj % LEAF_ELEMENTS(state);
		FreeEntry(state,&leafpage[off]);
		FREE_OBJ(leafpage[off]);
		leafpage[off].size = 0;
		UpdatePage(state,page);

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
		if		(  data->use  ==  0  ) {
			data->use =  sizeof(BLOB_V2_DataPage);
		}
		left = state->space->pagesize
			- ( data->use + sizeof(BLOB_V2_DataEntry) );
		if		(  left  <  ent->use  ) {
			csize = left;
		} else {
			csize = ent->use;
		}
		if		(  csize  >  0  ) {
			el = (BLOB_V2_DataEntry *)((byte *)data + data->use);
			pos = OBJ_POS(state,page,data->use);
			el->size = csize;
			el->next = 0;
			if		(  ent->last  !=  ent->head  ) {
				el->prio = ent->last;
			} else {
				el->prio = ent->node;
				OBJ_POINT_NODE(el->prio);
			}
			memcpy((byte *)el + sizeof(BLOB_V2_DataEntry),ent->buff,csize);
			memmove(ent->buff,ent->buff + csize,
					(state->space->pagesize - csize));
			ent->use -= csize;
			data->use += ROUND_TO(csize,sizeof(size_t))
				+ sizeof(BLOB_V2_DataEntry);
			UpdatePage(state,page);
			if		(  state->space->pagesize - data->use 
					   >  sizeof(BLOB_V2_DataEntry)  ) {
				ReturnPage(state,page);
			}
			if		(  ent->last  !=  ent->head  ) {
				wpage = OBJ_PAGE(state,ent->last);
				data = GetPage(state,wpage);
				wel = (BLOB_V2_DataEntry *)
					((byte *)data + OBJ_OFF(state,ent->last));
				wel->next = pos;
				UpdatePage(state,wpage);
			} else {
				ent->first = pos;
			}
			ent->last = pos;
		}
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
	dbgprintf("obj = %lld\n",obj);
	LockBLOB(state->space);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = 0;
		while	(  size  >  0  ) {
			left = state->space->pagesize - ent->use;
			csize = ( size < left ) ? size : left;
			memcpy(&ent->buff[ent->use],buff,csize);
			ent->use += csize;
			ent->size += csize;
			ret += csize;
			size -= csize;
			buff += csize;
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
		page = OBJ_PAGE(state,ent->last);
		off = OBJ_OFF(state,ent->last);
		dbgprintf("page = %lld\n",page);
		dbgprintf("off = %d\n",off);
		data = GetPage(state,page);
		el = (BLOB_V2_DataEntry *)((byte *)data + off);
		memcpy(ent->buff,(byte *)el + sizeof(BLOB_V2_DataEntry),el->size);
		ent->bsize = el->size;
		dbgprintf("el->size = %d\n",el->size);
		ent->last = el->next;
		ent->use = 0;
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
	dbgprintf("obj = %lld\n",obj);
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
