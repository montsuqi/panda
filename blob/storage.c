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
#include	<glib.h>
#include	<pthread.h>
#include	"types.h"
#include	"libmondai.h"
#include	"pagestruct.h"
#include	"lock.h"
#include	"storage.h"
#include	"table.h"
#include	"page.h"
#include	"message.h"
#include	"debug.h"

typedef	struct {
	ObjectType	obj;
	byte		mode;
	objpos_t	node;
	objpos_t	head;
	objpos_t	first;
	objpos_t	last;
	uint64_t	size;
	uint64_t	off;
	byte		*buff;
	size_t		bsize;
	size_t		use;
	int			tid;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	ObjectInfo;

static	void	WriteBuffer(OsekiSession *state, ObjectInfo *ent);

static	ObjectType
GetFreeOID(
	OsekiSession	*state)
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
	ObjectType	base;
	OsekiSpace	*space;
	OsekiObjectEntry	*leafpage;
	Bool		fFree;

ENTER_FUNC;
	space = state->space;
	leaf = 0;
	base = 0;
	Lock(&space->obj);
	while	(  leaf  ==  0  ) {
		for	( i = 0 ; i < state->objs->level ; i ++ ) {
			for	( j = 0 ; j < state->objs->level ; j ++ ) {
				stack[j] = state->objs->pos[j];
			}
			page = state->objs->pos[i];
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
								nodepage = UpdatePage(state,page);
								lower = NewPage(state,1);
								if		(  k  >  0  ) {
									nodepage[j] = lower | PAGE_NODE;
								} else {
									nodepage[j] = lower;
								}
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
			node = NewPage(state,1);
			(void)GetPage(state,node);
			nodepage = UpdatePage(state,node);
			nodepage[0] = state->objs->pos[state->objs->level-1] | PAGE_NODE;
			state->objs = UpdatePage(state,1LL);
			state->objs->pos[state->objs->level] = node;
			stack[state->objs->level] = node;
			state->objs->level ++;

		}
	}
	(void)GetPage(state,leaf);
	leafpage = UpdatePage(state,leaf);
	off = -1;
	use = 0;
	for	( j = 0 ; j < LEAF_ELEMENTS(state) ; j ++ ) {
		if		(  IS_FREEOBJ(leafpage[j])  )	{
			if		(  off  <  0  ) {
				USE_OBJ(leafpage[j]);
				leafpage[j].mode = OSEKI_OPEN_CLOSE;
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
	if		(  use  ==  LEAF_ELEMENTS(state)  ) {
		own = leaf;
		use = 0;
		for	( i = 0 ; i < state->objs->level ; i ++ ) {
			page = stack[i];
			(void)GetPage(state,page);
			nodepage = UpdatePage(state,page);
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
			if		(  fFree  )	break;
			if		(  PAGE_NO(state->objs->pos[i])  ==  page  ) {
				state->objs = UpdatePage(state,1LL);
				USE_NODE(state->objs->pos[i]);
			}
		}
	}
	UnLock(&space->obj);
LEAVE_FUNC;
	return	(base * LEAF_ELEMENTS(state) + off);
}

static	pageno_t
SearchLeafPage(
	OsekiSession	*state,
	ObjectType		obj,
	pageno_t		*path)
{
	pageno_t	off
		,		base
		,		page
		,		next
		,		*nodepage;
	OsekiSpace	*space;
	int			i;

ENTER_FUNC;
	space = state->space;
	base = obj / LEAF_ELEMENTS(state);
	page = 0;
	dbgprintf("level = %d\n",state->objs->level);
	for	( i = state->objs->level - 1 ; i >= 0 ; i -- ) {
		off = base / space->mul[i];
		if		(  page  ==  0  ) {
			page = state->objs->pos[i];
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

static	void
FreeEntry(
	OsekiSession		*state,
	OsekiObjectEntry	*ent)
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
	OsekiDataPage	*data
		,			*wd;
	OsekiDataEntry	*el
		,			*wel;
	OsekiObjectEntry	*leafnode;

ENTER_FUNC;
	dbgprintf("ent->flags = %X\n",(int)ent->flags);
	dbgprintf("ent->mode  = %X\n",(int)ent->mode);
	dbgprintf("ent->pos   = %lld\n",ent->pos);
	dbgprintf("ent->size  = %lld\n",ent->size);
	pos = ent->pos;
	while	(  pos  !=  0  ) {
		page = OBJ_PAGE(state,pos);
		(void)GetPage(state,page);
		data = UpdatePage(state,page);
		off = OBJ_OFF(state,pos);
		shlink = 0;
		for	( cur = sizeof(OsekiDataPage) ; cur < data->use ;  ) {
			el = (OsekiDataEntry *)((byte *)data + cur);
			nsize = ROUND_TO(el->size,sizeof(size_t))
				+ sizeof(OsekiDataEntry);
			if		(  cur  ==  off  ) {
				pos = el->next;
				memmove((byte *)data + cur,
					   (byte *)data + cur + nsize,
						data->use - cur - nsize);
				data->use -= nsize;
				memclear((byte *)data+data->use,PAGE_SIZE(state)-data->use);
				off = 0;
				el = (OsekiDataEntry *)((byte *)data + cur);
				nsize = ROUND_TO(el->size,sizeof(size_t))
					+ sizeof(OsekiDataEntry);
			}
			if		(  cur  ==  data->use  )	break;
#if	0
			printf("el->size = %d\n",el->size);
			printf("el->prio = %lld:%d\n",
				   OBJ_PAGE(state,el->prio),OBJ_OFF(state,el->prio));
			printf("el->next = %lld:%d\n",
				   OBJ_PAGE(state,el->next),OBJ_OFF(state,el->next));
#endif
			if		(  cur  >  off  ) {
				npos = OBJ_POS(state,page,cur);
				if		(  !IS_OBJ_NODE(el->prio)  ) {
					ppage = OBJ_PAGE(state,el->prio);
					(void)GetPage(state,ppage);
					wd = UpdatePage(state,ppage);
					wel = (OsekiDataEntry *)((byte *)wd
											 + OBJ_OFF(state,el->prio));
					wel->next = npos;
				} else {
					leaf = OBJ_PAGE(state,el->prio);
					(void)GetPage(state,leaf);
					leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
					leafnode[OBJ_OFF(state,el->prio)].pos = npos;
				}
				if		(  el->next  >  0  ) {
					npage = OBJ_PAGE(state,el->next);
					(void)GetPage(state,npage);
					wd = UpdatePage(state,npage);
					wel = (OsekiDataEntry *)((byte *)wd
											 + OBJ_OFF(state,el->next));
					wel->prio = npos;
				}
			}
			cur += nsize;
		}
		if		(  PAGE_SIZE(state) - data->use  >  sizeof(OsekiDataEntry)  ) {
			ReturnPage(state,page);
		}
	}
LEAVE_FUNC;
}

static	ObjectInfo	*
OpenEntry(
	OsekiSession	*state,
	pageno_t		leaf,
	ObjectType		obj,
	byte			mode)
{
	OsekiObjectEntry	*leafnode;
	int			off;
	ObjectInfo	*ent;

ENTER_FUNC;
	Lock(&state->space->obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  ==  NULL  ) {
		leafnode = (OsekiObjectEntry *)GetPage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		if		(  IS_FREEOBJ(leafnode[off])  ) {
			ent = NULL;
		} else
		if		(	(  leafnode[off].mode  ==  OSEKI_OPEN_CLOSE  )
				||	(	(  leafnode[off].mode  ==  OSEKI_OPEN_READ  )
					&&	(  mode                ==  OSEKI_OPEN_READ  ) ) ) {
			leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
			if		(  ( mode & OSEKI_OPEN_WRITE )  !=  0  ) {
				FreeEntry(state,&leafnode[off]);
				leafnode[off].pos = 0;
				leafnode[off].size = 0;
				leafnode[off].mode = mode & OSEKI_MODE_MASK;
				if		(  ( mode & OSEKI_ALLOC_LINER )  !=  0  ) {
					LINER_OBJ(leafnode[off]);
				} else {
					PACK_OBJ(leafnode[off]);
				}
			}
			if		(  ( mode & OSEKI_OPEN_READ )  !=  0  ) {
				if		(  IS_LINER(leafnode[off])  ) {
					mode |= OSEKI_ALLOC_LINER;
				}
			}
			if		(	(  leafnode[off].pos             !=  0  )
					&&	(  ( mode & OSEKI_OPEN_CREATE )  !=  0  ) ) {
				/*	remove	*/
				leafnode[off].pos = 0;
				leafnode[off].size = 0;
			}
			ent = New(ObjectInfo);
			ent->obj = obj;
			ent->mode = mode;
			ent->head = leafnode[off].pos;
			ent->first = leafnode[off].pos;
			ent->last = ent->head;
			ent->size = leafnode[off].size;
			ent->buff = xmalloc(state->space->pagesize);
			ent->bsize = 0;
			ent->use = 0;
			ent->off = 0;
			ent->node = OBJ_POS(state,leaf,off);
			ent->tid = state->tid;
			InitLock(ent);
			g_hash_table_insert(state->oTable,&ent->obj,ent);
#ifdef	DEBUG
			printf("ent->obj   = %lld\n",ent->obj);
			printf("ent->head  = %lld\n",ent->head);
			printf("ent->first = %lld\n",ent->first);
			printf("ent->last  = %lld\n",ent->last);
			printf("ent->size  = %lld\n",ent->size);
#endif
		} else {
			ent = NULL;
		}
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(ent);
}

extern	ObjectType
NewObject(
	OsekiSession	*state,
	int				mode)
{
	ObjectType	obj;
	pageno_t	leaf;

ENTER_FUNC;
	state->objs = GetPage(state,1LL);
	obj = GetFreeOID(state);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  OpenEntry(state,leaf,obj,(byte)mode|OSEKI_OPEN_CREATE)
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
OpenObject(
	OsekiSession	*state,
	ObjectType	obj,
	int				mode)
{
	pageno_t	leaf;
	ssize_t		ret;
	ObjectInfo	*ent;

ENTER_FUNC;
	state->objs = GetPage(state,1LL);
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
LEAVE_FUNC;
	return	(ret);
}

static	void
FlushObject(
	OsekiSession	*state,
	ObjectInfo	*ent)
{
	if		(  ( ent->mode & OSEKI_OPEN_WRITE )  !=  0  ) {
		while	(  ent->use  >  0  ) {
			WriteBuffer(state,ent);
		}
	}
}

static	void
CloseEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	ObjectType		obj)
{
	int				off;
	pageno_t		leaf;
	OsekiObjectEntry	*leafnode;

	FlushObject(state,ent);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		(void)GetPage(state,leaf);
		leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		if		(  ( leafnode[off].mode & OSEKI_OPEN_WRITE )  !=  0  ) {
			leafnode[off].size = ent->size;
			leafnode[off].pos = ent->first;
		}
		leafnode[off].mode = OSEKI_OPEN_CLOSE;
	}
	g_hash_table_remove(state->oTable,&obj);
	xfree(ent->buff);
	xfree(ent);
}

extern	Bool
CloseObject(
	OsekiSession	*state,
	ObjectType	obj)
{
	Bool			ret;
	ObjectInfo		*ent;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	ret = TRUE;
	Lock(&state->space->obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  !=  NULL  ) {
		CloseEntry(state,ent,obj);
	} else {
		ret = FALSE;
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
DestroyObject(
	OsekiSession	*state,
	ObjectType	obj)
{
	int				i
		,			j;
	OsekiObjectEntry	*leafpage;
	pageno_t		*nodepage;
	Bool			rc;
	pageno_t		stack[MAX_PAGE_LEVEL];
	pageno_t		page
		,			own;
	size_t			off;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	rc = TRUE;
	Lock(&state->space->obj);
	page = SearchLeafPage(state,obj,stack);
	if		(  page  ==  0  ) {
		rc = FALSE;
	} else {
		(void)GetPage(state,page);
		leafpage = UpdatePage(state,page);
		off = obj % LEAF_ELEMENTS(state);
		FreeEntry(state,&leafpage[off]);
		FREE_OBJ(leafpage[off]);

		own = page;
		for	( i = 0 ; i < state->objs->level ; i ++ ) {
			page = stack[i];
			(void)GetPage(state,page);
			nodepage = UpdatePage(state,page);
			for	( j = 0 ; j < NODE_ELEMENTS(state) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					FREE_NODE(nodepage[j]);
					break;
				}
			}
			own = page;
			if			(  PAGE_NO(state->objs->pos[i])  ==  page  ) {
				FREE_NODE(state->objs->pos[i]);
			}
			if		(  HAVE_FREECHILD(page)  )	break;
		}
		rc = TRUE;
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(rc);
}

static	void
WriteBuffer(
	OsekiSession	*state,
	ObjectInfo		*ent)
{
	pageno_t		page
		,			wpage
		,			pages
		,			from
		,			ip;
	OsekiDataPage	*data
		,			*fd;
	OsekiDataEntry	*el
		,			*wel;
	objpos_t		pos;
	size_t			csize
		,			left;

ENTER_FUNC;
	if		(  ( ent->mode & OSEKI_ALLOC_LINER )  ==  0  ) {
		page = GetFreePage(state);
		(void)GetPage(state,page);
		data = UpdatePage(state,page);
		if		(  data->use  ==  0  ) {
			data->use =  sizeof(OsekiDataPage);
		}
		left = state->space->pagesize - ( data->use + sizeof(OsekiDataEntry) );
		if		(  left  <  ent->use  ) {
			csize = left;
		} else {
			csize = ent->use;
		}
		if		(  csize  >  0  ) {
printf("data->use = %d\n",data->use);
			el = (OsekiDataEntry *)((byte *)data + data->use);
			pos = OBJ_POS(state,page,data->use);
			el->size = csize;
			el->next = 0;
			if		(  ent->last  !=  ent->head  ) {
				el->prio = ent->last;
			} else {
				el->prio = ent->node;
				OBJ_POINT_NODE(el->prio);
			}
			memcpy((byte *)el + sizeof(OsekiDataEntry),ent->buff,csize);
			memmove(ent->buff,ent->buff + csize,
					(PAGE_SIZE(state) - csize));
			ent->use -= csize;
			data->use += ROUND_TO(csize,sizeof(size_t))
				+ sizeof(OsekiDataEntry);
			if		(  PAGE_SIZE(state) - data->use 
					   >  sizeof(OsekiDataEntry)  ) {
				ReturnPage(state,page);
			}
			if		(  ent->last  !=  ent->head  ) {
				wpage = OBJ_PAGE(state,ent->last);
				(void)GetPage(state,wpage);
				data = UpdatePage(state,wpage);
				wel = (OsekiDataEntry *)
					((byte *)data + OBJ_OFF(state,ent->last));
				wel->next = pos;
			} else {
				ent->first = pos;
			}
			ent->last = pos;
			ent->off += csize;
		}
	} else {
printf("ent->first = %lld\n",OBJ_PAGE(state,ent->first));
printf("ent->last  = %lld\n",OBJ_PAGE(state,ent->last));
		if		(	(  ent->last   >  0  )
				&&	(  ent->first  >  0  ) ) {
			pages = OBJ_PAGE(state,ent->last) - OBJ_PAGE(state,ent->first) + 1;
			printf("pages = %lld\n",pages);
			page = NewPage(state,pages+1);
			wpage = page;
			from = OBJ_PAGE(state,ent->first);
			for	( ip = 0 ; ip < pages ; ip ++ , from ++ , page ++ ) {
				fd = GetPage(state,from);
				(void)GetPage(state,page);
				data = UpdatePage(state,page);
				memcpy(data,fd,PAGE_SIZE(state));
				fd = UpdatePage(state,from);
				memclear(fd,PAGE_SIZE(state));
				ReturnPage(state,from);
			}
		} else {
			page = NewPage(state,1);
			wpage = page;
		}
		(void)GetPage(state,page);
		data = UpdatePage(state,page);
		memcpy(data,ent->buff,ent->use);
		ent->off += ent->use;
		ent->first = OBJ_POS(state,wpage,0);
		ent->last = OBJ_POS(state,page,0);
		ent->use = 0;
	}
LEAVE_FUNC;
}

static	ssize_t
WriteEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	byte			*buff,
	size_t			size)
{
	ssize_t		ret;
	size_t		csize
		,		left;

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
	return	(ret);
}

static	void
ReadBuffer(
	OsekiSession	*state,
	ObjectInfo		*ent)
{
	pageno_t			page;
	OsekiDataPage	*data;
	OsekiDataEntry	*el;
	size_t			off;

ENTER_FUNC;
	if		(  ent->last  >  0  ) {
		page = OBJ_PAGE(state,ent->last);
		off = OBJ_OFF(state,ent->last);
		dbgprintf("page = %lld\n",page);
		dbgprintf("off = %d\n",off);
		data = GetPage(state,page);
		if		(  ( ent->mode & OSEKI_ALLOC_LINER )  ==  0  ) {
			el = (OsekiDataEntry *)((byte *)data + off);
			memcpy(ent->buff,(byte *)el + sizeof(OsekiDataEntry),el->size);
			ent->bsize = el->size;
			dbgprintf("el->size = %d\n",el->size);
			ent->last = el->next;
		} else {
			if		(  data  !=  NULL  ) {
				if		(  off  ==  0  )	off = PAGE_SIZE(state);
				memcpy(ent->buff,data,PAGE_SIZE(state));
				ent->bsize = PAGE_SIZE(state);
				ent->last += PAGE_SIZE(state);
			} else {
				ent->bsize = 0;
			}
		}
		ent->use = 0;
	} else {
		ent->bsize = 0;
	}
LEAVE_FUNC;
}

static	ssize_t
ReadEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	byte			*buff,
	size_t			size)
{
	ssize_t		ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	ret = 0;
printf("ent->use  = %d\n",ent->use);
printf("ent->off  = %lld\n",ent->off);
printf("ent->size = %lld\n",ent->size);
	while	(	(  size      >  0          )
			&&	(  ent->off  <  ent->size  ) ) {
		left = ent->bsize - ent->use;
		if		(  left  ==  0  ) {
			ReadBuffer(state,ent);
			if		(  ent->bsize  ==  0  )	break;
			left = ( ent->bsize > ( ent->size - ent->off ) ) ?
				( ent->size - ent->off ) : ent->bsize;
		}
		csize = ( size < left ) ? size : left;
		memcpy(buff,&ent->buff[ent->use],csize);
		ent->use += csize;
		size -= csize;
		ret += csize;
		buff += csize;
		ent->off += csize;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	ObjectType
InitiateObject(
	OsekiSession	*state,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	ObjectType	obj;
	pageno_t	leaf;

ENTER_FUNC;
	state->objs = GetPage(state,1LL);
	obj = GetFreeOID(state);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  ( ent = OpenEntry(state,leaf,obj,
									 OSEKI_OPEN_WRITE|OSEKI_OPEN_CREATE) )  ==  NULL  ) {
			obj = GL_OBJ_NULL;
		}
	} else {
		ent  = NULL;
		obj = GL_OBJ_NULL;
	}
	if		(  obj  !=  GL_OBJ_NULL  ) {
		WriteEntry(state,ent,buff,size);
		CloseEntry(state,ent,obj);
	}
LEAVE_FUNC;
	return	(obj);
}

extern	ssize_t
GetObject(
	OsekiSession	*state,
	ObjectType		obj,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	ssize_t		ret;

ENTER_FUNC;
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		ent = OpenEntry(state,leaf,obj,OSEKI_OPEN_READ);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		if		(  size  ==  0  ) {
			size = ent->size;
		} else {
			size = ( size  <  ent->size ) ? size : ent->size;
		}
		ReadEntry(state,ent,buff,size);
		CloseEntry(state,ent,obj);
		ret = size;
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	ssize_t
PutObject(
	OsekiSession	*state,
	ObjectType		obj,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	OsekiObjectEntry	*leafpage;
	ssize_t		ret;
	size_t		off;

ENTER_FUNC;
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		Lock(&state->space->obj);
		(void)GetPage(state,leaf);
		leafpage = UpdatePage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		FreeEntry(state,&leafpage[off]);
		leafpage[off].mode = OSEKI_OPEN_CLOSE;
		leafpage[off].pos = 0;
		leafpage[off].size = 0;
		USE_OBJ(leafpage[off]);
		UnLock(&state->space->obj);
		ent = OpenEntry(state,leaf,obj,OSEKI_OPEN_WRITE);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		WriteEntry(state,ent,buff,size);
		CloseEntry(state,ent,obj);
		ret = size;
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	int
WriteObject(
	OsekiSession	*state,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	ssize_t		ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = WriteEntry(state,ent,buff,size);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	int
ReadObject(
	OsekiSession	*state,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	int			ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = ReadEntry(state,ent,buff,size);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ssize_t
SeekEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	uint64_t		pos)
{
	ssize_t		ret;

	ret = 0;
	if		(  ( ent->mode & OSEKI_ALLOC_LINER )  ==  0  ) {
		ent->last = ent->head;
		ent->off = 0;
		while	(	(  ent->off  <  pos        )
				&&	(  ent->off  <  ent->size  ) ) {
printf("ent->off   = %lld\n",ent->off);
			ReadBuffer(state,ent);
printf("ent->bsize = %d\n",ent->bsize);
			if		(  ent->bsize  ==  0  )	break;
			if		(  ent->off + ent->bsize >  pos  ) {
				ent->use = pos - ent->off;
				ent->off = pos;
			} else {
				ent->off += ent->bsize;
			}
		}
	} else {
		ent->last = ent->head + pos;
		ent->off = pos;
		ent->use = OBJ_OFF(state,pos);
	}
printf("ent->use  = %d\n",ent->use);
printf("ent->off  = %lld\n",ent->off);
printf("ent->size = %lld\n",ent->size);
	return	(ret);
}

extern	int
SeekObject(
	OsekiSession	*state,
	ObjectType		obj,
	uint64_t		pos)
{
	ObjectInfo	*ent;
	int			ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = SeekEntry(state,ent,pos);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
OsekiTransactionStart(
	OsekiSession	*state)
{
	int		tid;

ENTER_FUNC;
	Lock(state->space);
	tid = state->space->cTran ++;
	state->tid = tid;
	g_tree_insert(state->space->trans,&state->tid,state);
	UnLock(state->space);
	state->oTable = NewLLHash();
	state->pages = NewLLHash();
	state->objs = NULL;
LEAVE_FUNC;
	return	(TRUE);
}

static	void
SyncObject(
	ObjectType		*obj,
	ObjectInfo		*ent,
	OsekiSession	*state)
{
ENTER_FUNC;
	xfree(ent->buff);
	xfree(ent);
LEAVE_FUNC;
}

static	void
CommitObject(
	OsekiSession	*state)
{
ENTER_FUNC;
	g_hash_table_foreach(state->oTable,(GHFunc)SyncObject,state);
	g_hash_table_destroy(state->oTable);
	state->oTable = NULL;
LEAVE_FUNC;
}

static	void
SetLeast(
	OsekiSession	*state)
{
	int		_least(
		int				tid,
		OsekiSession	*ses,
		OsekiSpace		*space)
	{
		space->lTran = ses->tid;
		printf("tid = %d\n",space->lTran);
		return	(TRUE);
	}
ENTER_FUNC;
	g_tree_remove(state->space->trans,(gpointer)state->tid);
	g_tree_traverse(state->space->trans,(GTraverseFunc)_least,
					G_IN_ORDER,state->space);
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionCommit(
	OsekiSession	*state)
{
ENTER_FUNC;
	SetLeast(state);
	CommitObject(state);
	CommitPages(state);
	state->tid = 0;
LEAVE_FUNC;
	return	(TRUE);
}

static	void
FreeObject(
	ObjectType		*obj,
	ObjectInfo		*ent,
	OsekiSession	*state)
{
ENTER_FUNC;
	xfree(ent->buff);
	xfree(ent);
LEAVE_FUNC;
}

static	void
AbortObject(
	OsekiSession	*state)
{
ENTER_FUNC;
	g_hash_table_foreach(state->oTable,(GHFunc)FreeObject,state);
	g_hash_table_destroy(state->oTable);
	state->oTable = NULL;
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionAbort(
	OsekiSession	*state)
{
ENTER_FUNC;
	SetLeast(state);
	AbortObject(state);
	AbortPages(state);
	state->tid = 0;
LEAVE_FUNC;
	return	(TRUE);
}
