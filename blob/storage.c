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
	objpos_t	curr;
	uint64_t	size;
	uint64_t	off;
	byte		*buff;
	size_t		bsize;
	size_t		use;
	int			tid;
	int			depth;
	Bool		fDurty;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	ObjectInfo;

static	void	FlushBuffer(OsekiSession *ses, ObjectInfo *ent, Bool fOnly);

#ifdef	DEBUG
static	void
_DumpObject(
	OsekiSession		*ses,
	OsekiObjectEntry	*ent)
{
	printf("ent->use    = %s\n",ent->use ? "TRUE" : "FALSE");
	printf("ent->mode   = %X\n",(int)ent->mode);
	printf("ent->type   = %X\n",(int)ent->type);
	if		(  ent->type  ==  OSEKI_ALLOC_SHORT  ) {
	} else {
		printf("ent->pos    = %lld:%d\n",
			   OBJ_PAGE(ses,ent->body.pointer.pos),
			   OBJ_OFF(ses,ent->body.pointer.pos));
		printf("ent->size   = %lld\n",ent->body.pointer.size);
	}
}
#define	DumpObject(s,ent)		_DumpObject((s),(ent))

static	void
_DumpInfo(
	OsekiSession	*ses,
	ObjectInfo		*ent)
{
	printf("ent->obj    = %lld\n",ent->obj);
	printf("ent->mode   = %02X\n",ent->mode);
	printf("ent->size   = %lld\n",ent->size);
	printf("ent->use    = %d\n",ent->use);
	printf("ent->off    = %lld\n",ent->off);
	printf("ent->bsize  = %d\n",ent->bsize);
	printf("ent->node   = %lld:%d\n",
		   OBJ_PAGE(ses,ent->node),OBJ_OFF(ses,ent->node));
	printf("ent->head   = %lld:%d\n",
		   OBJ_PAGE(ses,ent->head),OBJ_OFF(ses,ent->head));
	printf("ent->curr   = %lld:%d\n",
		   OBJ_PAGE(ses,ent->curr),OBJ_OFF(ses,ent->curr));
	printf("ent->fDurty = %s\n",ent->fDurty ? "TRUE" : "FALSE");
	printf("ent->depth  = %d\n",ent->depth);
}
#define	DumpInfo(s,ent)		_DumpInfo((s),(ent))

static	void
_DumpEntry(
	OsekiSession	*ses,
	OsekiDataEntry	*el)
{
	printf("el->size = %d\n",el->size);
	printf("el->prio = %lld:%d\n",
		   OBJ_PAGE(ses,el->prio),OBJ_OFF(ses,el->prio));
	printf("el->next = %lld:%d\n",
		   OBJ_PAGE(ses,el->next),OBJ_OFF(ses,el->next));
}
#define	DumpEntry(s,el)		_DumpEntry((s),(el))
#define	DumpPage(s,d)		_DumpPage((s)->space,(byte *)(d))
#else
#define	DumpObject(s,ent)	/*	*/
#define	DumpInfo(s,ent)		/*	*/
#define	DumpEntry(s,el)		/*	*/
#define	DumpPage(s,d)		/*	*/
#endif

static	ObjectType
GetFreeOID(
	OsekiSession	*ses)
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
	space = ses->space;
	leaf = 0;
	base = 0;
	Lock(&space->obj);
	while	(  leaf  ==  0  ) {
		for	( i = 0 ; i < ses->objs->level ; i ++ ) {
			for	( j = 0 ; j < ses->objs->level ; j ++ ) {
				stack[j] = ses->objs->pos[j];
			}
			page = ses->objs->pos[i];
			if		(  HAVE_FREECHILD(page)  ) {
				base = 0;
				for	( k = i ; k >= 0 ; k -- ) {
					stack[k] = page;
					nodepage = GetPage(ses,page);
					lower = page;
					node = 0;
					for	( j = 0 ; j <  NODE_ELEMENTS(ses) ; j ++ ) {
						if		(  HAVE_FREECHILD(nodepage[j])  ) {
							if		(  ( lower = PAGE_NO(nodepage[j]) )  ==  0  ) {
								nodepage = UpdatePage(ses,page);
								lower = NewPage(ses,1);
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
					base = base + node * ses->space->mul[k];
				}
				if		(  leaf  >  0  )	break;
			}
		}
		if		(  leaf  ==  0  ) {
			node = NewPage(ses,1);
			(void)GetPage(ses,node);
			nodepage = UpdatePage(ses,node);
			nodepage[0] = ses->objs->pos[ses->objs->level-1] | PAGE_NODE;
			ses->objs = UpdatePage(ses,1LL);
			ses->objs->pos[ses->objs->level] = node;
			stack[ses->objs->level] = node;
			ses->objs->level ++;

		}
	}
	(void)GetPage(ses,leaf);
	leafpage = UpdatePage(ses,leaf);
	off = -1;
	use = 0;
	for	( j = 0 ; j < LEAF_ELEMENTS(ses) ; j ++ ) {
		if		(  IS_FREEOBJ(leafpage[j])  )	{
			if		(  off  <  0  ) {
				USE_OBJ(leafpage[j]);
				leafpage[j].mode = OSEKI_OPEN_CLOSE;
				leafpage[j].body.pointer.pos = 0;
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
	if		(  use  ==  LEAF_ELEMENTS(ses)  ) {
		own = leaf;
		use = 0;
		for	( i = 0 ; i < ses->objs->level ; i ++ ) {
			page = stack[i];
			(void)GetPage(ses,page);
			nodepage = UpdatePage(ses,page);
			fFree = FALSE;
			for	( j = 0 ; j < NODE_ELEMENTS(ses) ; j ++ ) {
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
			if		(  PAGE_NO(ses->objs->pos[i])  ==  page  ) {
				ses->objs = UpdatePage(ses,1LL);
				USE_NODE(ses->objs->pos[i]);
			}
		}
	}
	UnLock(&space->obj);
LEAVE_FUNC;
	return	(base * LEAF_ELEMENTS(ses) + off);
}

static	pageno_t
SearchLeafPage(
	OsekiSession	*ses,
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
	space = ses->space;
	base = obj / LEAF_ELEMENTS(ses);
	page = 0;
	dbgprintf("level = %d\n",ses->objs->level);
	for	( i = ses->objs->level - 1 ; i >= 0 ; i -- ) {
		off = base / space->mul[i];
		if		(  page  ==  0  ) {
			page = ses->objs->pos[i];
		}
		if		(  path  !=  NULL  ) {
			path[i] = page;
		}
		nodepage = GetPage(ses,page);
		next = PAGE_NO(nodepage[off]);
		page = next;
		base %= NODE_ELEMENTS(ses);
	}
LEAVE_FUNC;
	return	(page);
}

static	void
FreeEntry(
	OsekiSession		*ses,
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
	pos = ent->body.pointer.pos;
	switch	(ent->mode & OSEKI_TYPE_MASK) {
	  case	OSEKI_ALLOC_LINER:
		dbgmsg("*");
		break;
	  case	OSEKI_ALLOC_TREE:
		dbgmsg("*");
		break;
	  case	OSEKI_ALLOC_PACK:
		dbgmsg("*");
		DumpObject(ses,ent);
		while	(  pos  !=  0  ) {
			page = OBJ_PAGE(ses,pos);
			(void)GetPage(ses,page);
			data = UpdatePage(ses,page);
			off = OBJ_OFF(ses,pos);
			shlink = 0;
			for	( cur = sizeof(OsekiDataPage) ; cur < data->use ;  ) {
				el = (OsekiDataEntry *)((byte *)data + cur);
				nsize = ROUND_TO(el->size+sizeof(OsekiDataEntry),
								 SIZE_RESOLUTION);
				DumpEntry(ses,el);
				if		(  cur  ==  off  ) {
					pos = el->next;
					memmove((byte *)data + cur,
							(byte *)data + cur + nsize,
							data->use - cur - nsize);
					data->use -= nsize;
					memclear((byte *)data+data->use,
							 PAGE_SIZE(ses)-data->use);
					off = 0;
					el = (OsekiDataEntry *)((byte *)data + cur);
					nsize = ROUND_TO(el->size,SIZE_RESOLUTION)
						+ sizeof(OsekiDataEntry);
				}
				if		(  cur  ==  data->use  )	break;
				DumpEntry(ses,el);
				if		(  cur  >  off  ) {
					npos = OBJ_POS(ses,page,cur);
					if		(  !IS_OBJ_NODE(el->prio)  ) {
						ppage = OBJ_PAGE(ses,el->prio);
						(void)GetPage(ses,ppage);
						wd = UpdatePage(ses,ppage);
						wel = (OsekiDataEntry *)((byte *)wd
												 + OBJ_OFF(ses,el->prio));
						wel->next = npos;
					} else {
						leaf = OBJ_PAGE(ses,el->prio);
						(void)GetPage(ses,leaf);
						leafnode = (OsekiObjectEntry *)UpdatePage(ses,leaf);
						leafnode[OBJ_OFF(ses,el->prio)].body.pointer.pos = npos;
					}
					if		(  el->next  >  0  ) {
						npage = OBJ_PAGE(ses,el->next);
						(void)GetPage(ses,npage);
						wd = UpdatePage(ses,npage);
						wel = (OsekiDataEntry *)((byte *)wd
												 + OBJ_OFF(ses,el->next));
						wel->prio = npos;
					}
				}
				cur += nsize;
			}
			if		(  PAGE_SIZE(ses) - data->use
					   >  sizeof(OsekiDataEntry) + SIZE_RESOLUTION  ) {
				ReturnPage(ses,page);
			}
			break;
		  case	OSEKI_ALLOC_SHORT:
			dbgmsg("*");
			break;
		}
	}
LEAVE_FUNC;
}

static	ObjectInfo	*
OpenEntry(
	OsekiSession	*ses,
	pageno_t		leaf,
	ObjectType		obj,
	byte			mode)
{
	OsekiObjectEntry	*leafnode;
	int			off;
	ObjectInfo	*ent;
	uint64_t	size;

ENTER_FUNC;
	Lock(&ses->space->obj);
	if		(  ( ent = g_hash_table_lookup(ses->oTable,&obj) )  ==  NULL  ) {
		leafnode = (OsekiObjectEntry *)GetPage(ses,leaf);
		off = obj % LEAF_ELEMENTS(ses);
		if		(  IS_FREEOBJ(leafnode[off])  ) {
			ent = NULL;
		} else
		if		(	(  ( leafnode[off].mode << 4 )     ==  OSEKI_OPEN_CLOSE  )
				||	(	(  ( leafnode[off].mode << 4)  ==  OSEKI_OPEN_READ  )
					&&	(  mode                ==  OSEKI_OPEN_READ  ) ) ) {
			if		(  ( leafnode[off].mode & OSEKI_OPEN_WRITE )  !=  0  ) {
				leafnode = (OsekiObjectEntry *)UpdatePage(ses,leaf);
			}
			if		( ( mode & OSEKI_OPEN_CREATE )  !=  0  ) {
				if		(  leafnode[off].body.pointer.pos  >  0  ) {
					FreeEntry(ses,&leafnode[off]);
				}
				leafnode[off].body.pointer.pos = 0;
				leafnode[off].body.pointer.size = 0;
				leafnode[off].mode = ( mode & OSEKI_MODE_MASK ) >> 4;
				switch	( mode & OSEKI_TYPE_MASK ) {
				  case	OSEKI_ALLOC_LINER:
					leafnode[off].type = BODY_LINER;
					break;
				  case	OSEKI_ALLOC_TREE:
					leafnode[off].type = BODY_TREE;
					break;
				  case	OSEKI_ALLOC_SHORT:
					leafnode[off].type = BODY_SHORT;
					break;
				  default:
					leafnode[off].type = BODY_PACK;
					break;
				}
			} else {
				mode = ( mode & OSEKI_MODE_MASK ) | leafnode[off].type;
			}
			ent = New(ObjectInfo);
			ent->obj = obj;
			ent->mode = mode;
			if		(  ( mode & OSEKI_TYPE_MASK )  ==  OSEKI_ALLOC_SHORT  ) {
				ent->head = 0;
				ent->curr = 0;
				ent->size = leafnode[off].ssize;
				ent->buff = (byte *)xmalloc(SIZE_SMALL);
				memcpy(ent->buff,leafnode[off].body.small,SIZE_SMALL);
				ent->bsize = SIZE_SMALL;
			} else {
				ent->head = leafnode[off].body.pointer.pos;
				ent->curr = ent->head;
				ent->size = leafnode[off].body.pointer.size;
				ent->buff = NULL;
				ent->bsize = 0;
			}
			ent->use = 0;
			ent->off = 0;
			ent->fDurty = FALSE;
			ent->node = OBJ_POS(ses,leaf,off);
			ent->tid = ses->tid;
			switch	( mode & OSEKI_TYPE_MASK ) {
			  case	OSEKI_ALLOC_TREE:
				ent->depth = 0;
				size = ent->size;
				size /= PAGE_SIZE(ses);
				while	(  size  >  0  ) {
					ent->depth ++;
					size /= NODE_ELEMENTS(ses);
				}
				break;
			  default:
				ent->depth = 0;
				break;
			}
			InitLock(ent);
			g_hash_table_insert(ses->oTable,&ent->obj,ent);
			DumpInfo(ses,ent);
		} else {
			ent = NULL;
		}
	}
	UnLock(&ses->space->obj);
LEAVE_FUNC;
	return	(ent);
}

static	void
FlushObject(
	OsekiSession	*ses,
	ObjectInfo	*ent)
{
	if		(  ( ent->mode & OSEKI_OPEN_WRITE )  !=  0  ) {
		if		(  ent->use  >  0  ) {
			FlushBuffer(ses,ent,TRUE);
		}
	}
}

static	void
CloseEntry(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	ObjectType		obj)
{
	int				off;
	pageno_t		leaf;
	OsekiObjectEntry	*leafnode;

ENTER_FUNC;
	FlushObject(ses,ent);
	DumpInfo(ses,ent);
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		(void)GetPage(ses,leaf);
		leafnode = (OsekiObjectEntry *)UpdatePage(ses,leaf);
		off = obj % LEAF_ELEMENTS(ses);
		if		(  ( leafnode[off].mode & ( OSEKI_OPEN_WRITE >> 4 ) )
				   !=  0  ) {
			if		(  leafnode[off].type  ==  OSEKI_ALLOC_SHORT  ) {
				memcpy(leafnode[off].body.small,ent->buff,SIZE_SMALL);
				leafnode[off].ssize = ent->size;
				xfree(ent->buff);
				ent->buff = NULL;
			} else {
				leafnode[off].body.pointer.size = ent->size;
				leafnode[off].body.pointer.pos = ent->head;
			}
		}
		leafnode[off].mode = OSEKI_OPEN_CLOSE >> 4;
	}
LEAVE_FUNC;
}

static	void
SeekTree(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	Bool			fWrite)
{
	int			depth2
		,		depth
		,		off;
	uint64_t	size
		,		base;
	pageno_t	page
		,		prio;
	pageno_t	*pages;

ENTER_FUNC;
	depth2 = 0;
	size = ent->off;
	size /= PAGE_SIZE(ses);
	while	(  size  >  0  ) {
		depth2 ++;
		size /= NODE_ELEMENTS(ses);
	}
	if		(  fWrite  ) {
		depth = ( depth2 > ent->depth ? depth2 : ent->depth );
		for	( ; depth2 > ent->depth ; depth2 -- ) {
			page = NewPage(ses,1);
			(void)GetPage(ses,page);
			pages = UpdatePage(ses,page);
			memclear(pages,PAGE_SIZE(ses));
			pages[0] = OBJ_PAGE(ses,ent->head);
			ent->head = OBJ_POS(ses,page,0);
		}
		ent->depth = depth;
	}
	page = OBJ_PAGE(ses,ent->head);
	base = ent->off / PAGE_SIZE(ses);
	prio = 0;
	off = 0;
	for	( depth = ent->depth ; depth > 0 ; depth -- ) {
		if		(  page  ==  0  ) {
			if		(  fWrite  ) {
				page = NewPage(ses,1);
				if		(  prio  ==  0  ) {
					ent->head = OBJ_POS(ses,page,0);
				} else {
					pages = UpdatePage(ses,prio);
					pages[off] = page;
				}
			} else
				break;
		}
		off = base / ses->space->mul[depth-1];
		pages = GetPage(ses,page);
		prio = page;
		page = pages[off];
		base %= NODE_ELEMENTS(ses);
	}
	if		(  page  ==  0  ) {
		if		(  fWrite  ) {
			page = NewPage(ses,1);
			if		(  prio  ==  0  ) {
				ent->head = OBJ_POS(ses,page,0);
			} else {
				pages = UpdatePage(ses,prio);
				pages[off] = page;
			}
			ent->curr = OBJ_POS(ses,page,OBJ_OFF(ses,ent->off));
		} else {
			ent->curr = GL_OBJ_NULL;
		}
	} else {
		ent->curr = OBJ_POS(ses,page,OBJ_OFF(ses,ent->off));
	}
LEAVE_FUNC;
}

static	void
ReadBuffer(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	Bool			fOnly)
{
	pageno_t			page;
	OsekiDataPage	*data;
	OsekiDataEntry	*el;
	size_t			off;

ENTER_FUNC;
	if		(  ent->curr  >  0  ) {
		switch	(ent->mode & OSEKI_TYPE_MASK) {
		  case	OSEKI_ALLOC_LINER:
			dbgmsg("*");
			page = OBJ_PAGE(ses,ent->curr);
			data = GetPage(ses,page);
			if		(  data  !=  NULL  ) {
				ent->buff = (byte *)data;
				ent->bsize = PAGE_SIZE(ses);
			} else {
				ent->bsize = 0;
			}
			ent->use = OBJ_OFF(ses,ent->curr);
			break;
		  case	OSEKI_ALLOC_TREE:
			dbgmsg("*");
			SeekTree(ses,ent,(ent->mode&OSEKI_OPEN_WRITE) != 0);
			if		(  ent->curr  >  0  ) {
				page = OBJ_PAGE(ses,ent->curr);
				data = GetPage(ses,page);
				if		(  data  !=  NULL  ) {
					ent->buff = (byte *)data;
					ent->bsize = PAGE_SIZE(ses);
				} else {
					ent->bsize = 0;
				}
			} else {
				ent->bsize = 0;
			}
			ent->use = OBJ_OFF(ses,ent->curr);
			break;
		  case	OSEKI_ALLOC_PACK:
			dbgmsg("*");
			page = OBJ_PAGE(ses,ent->curr);
			off = OBJ_OFF(ses,ent->curr);
			data = GetPage(ses,page);
			el = (OsekiDataEntry *)((byte *)data + off);
			ent->buff = (byte *)el + sizeof(OsekiDataEntry);
			ent->bsize = el->size;
			ent->use = 0;
			break;
		  case	OSEKI_ALLOC_SHORT:
			dbgmsg("*");
			break;
		  default:
			break;
		}
		if		(  !fOnly  ) {
			switch	(ent->mode & OSEKI_TYPE_MASK) {
			  case	OSEKI_ALLOC_LINER:
				ent->curr += PAGE_SIZE(ses);
				break;
			  case	OSEKI_ALLOC_PACK:
				ent->curr = el->next;
				break;
			  case	OSEKI_ALLOC_SHORT:
				break;
			  default:
				break;
			}
		}
	} else {
		ent->bsize = 0;
	}
	DumpInfo(ses,ent);
LEAVE_FUNC;
}

static	void
FlushBuffer(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	Bool			fOnly)
{
	pageno_t		page
		,			wpage;
	OsekiDataPage	*data;
	OsekiDataEntry	*el
		,			*wel;
	size_t			off;
	objpos_t		next;

ENTER_FUNC;
	if		(	(  ent->buff  ==  NULL  )
			||	(  ent->curr  ==  0  ) ) {
	} else
	if		(  ent->fDurty  ) {
		switch	(ent->mode & OSEKI_TYPE_MASK) {
		  case	OSEKI_ALLOC_LINER:
			dbgmsg("*");
			break;
		  case	OSEKI_ALLOC_TREE:
			dbgmsg("*");
			break;
		  case	OSEKI_ALLOC_PACK:
			dbgmsg("*");
			page = OBJ_PAGE(ses,ent->curr);
			off = OBJ_OFF(ses,ent->curr);
			data = GetPage(ses,page);
			el = (OsekiDataEntry *)((byte *)data + off);
			DumpEntry(ses,el);
			next = el->next;
			ent->buff = NULL;
			if		(  el->size  ==  0  ) {
				el->size = ent->use;
				data = UpdatePage(ses,page);
				data->use += ROUND_TO(ent->use+sizeof(OsekiDataEntry),
									  SIZE_RESOLUTION);
				if		(  ent->head  ==  0  ) {
					ent->head = ent->curr;
				} else {
					if		(  !IS_OBJ_NODE(el->prio)  ) {
						wpage = OBJ_PAGE(ses,el->prio);
						(void)GetPage(ses,wpage);
						data = UpdatePage(ses,wpage);
						wel = (OsekiDataEntry *)
							((byte *)data + OBJ_OFF(ses,el->prio));
						wel->next = ent->curr;
					}
				}
				if		(  ( PAGE_SIZE(ses) - data->use )
						   >  sizeof(OsekiDataEntry) + SIZE_RESOLUTION  ) {
					ReturnPage(ses,page);
				}
			}
			break;
		  case	OSEKI_ALLOC_SHORT:
			dbgmsg("*");
			break;
		  default:
			break;
		}
	}
	if		(  !fOnly  ) {
		switch	(ent->mode & OSEKI_TYPE_MASK) {
		  case	OSEKI_ALLOC_LINER:
			//ent->curr = ent->head + ent->use;
			//ent->curr = ent->head + ent->off + ent->use;
			//ent->curr = ent->head + ent->off;
			ent->curr += ent->bsize;
			break;
		  case	OSEKI_ALLOC_TREE:
			//ent->off += ent->use;
			SeekTree(ses,ent,TRUE);
			break;
		  case	OSEKI_ALLOC_PACK:
			ent->curr = next;
			break;
		  case	OSEKI_ALLOC_SHORT:
		  default:
			break;
		}
	}
	ent->fDurty = FALSE;
	DumpInfo(ses,ent);
LEAVE_FUNC;
}

static	void
NewBuffer(
	OsekiSession	*ses,
	ObjectInfo		*ent)
{
	pageno_t		page
		,			pages
		,			wpage
		,			from
		,			ip;
	OsekiDataPage	*data
		,			*fd;
	OsekiDataEntry	*el;
	uint64_t		size;

ENTER_FUNC;
	DumpInfo(ses,ent);
	switch	(ent->mode & OSEKI_TYPE_MASK) {
	  case	OSEKI_ALLOC_LINER:
		dbgmsg("*");
		wpage = 0;
		if		(  ent->head  >  0  ) {
			if		(  ent->off  <  ent->size  ) {
				page = OBJ_PAGE(ses,ent->head + ent->off);
			} else {
				size = ent->size > ent->off ? ent->size : ent->off;
				dbgprintf("size  = %lld\n",size);
				pages = OBJ_PAGE(ses,size);
				dbgprintf("pages = %lld\n",pages);
				page = NewPage(ses,pages+1);
				wpage = page;
				from = OBJ_PAGE(ses,ent->head);
				for	( ip = 0 ; ip < pages ; ip ++ , from ++ , page ++ ) {
					fd = GetPage(ses,from);
					(void)GetPage(ses,page);
					data = UpdatePage(ses,page);
					memcpy(data,fd,PAGE_SIZE(ses));
					fd = UpdatePage(ses,from);
					memclear(fd,PAGE_SIZE(ses));
					ReturnPage(ses,from);
				}
			}
		} else {
			page = NewPage(ses,1);
			wpage = page;
		}
dbgprintf("wpage = %lld\n",wpage);
		(void)GetPage(ses,page);
		ent->buff = UpdatePage(ses,page);
		DumpInfo(ses,ent);
		if		(  wpage  >  0  ) {
			ent->head = OBJ_POS(ses,wpage,0);
		}
		ent->curr = OBJ_POS(ses,page,0);
		ent->bsize = PAGE_SIZE(ses);
		break;
	  case	OSEKI_ALLOC_TREE:
		dbgmsg("*");
		SeekTree(ses,ent,TRUE);
		page = OBJ_PAGE(ses,ent->curr);
		(void)GetPage(ses,page);
		ent->buff = UpdatePage(ses,page);
		DumpInfo(ses,ent);
		break;
	  case	OSEKI_ALLOC_PACK:
		dbgmsg("*");
		page = GetFreePage(ses);
		(void)GetPage(ses,page);
		data = UpdatePage(ses,page);
		if		(  data->use  ==  0  ) {
			data->use =  sizeof(OsekiDataPage);
		}
		el = (OsekiDataEntry *)((byte *)data + data->use);
		el->size = 0;
		el->next = 0;
		el->prio = ent->node;
		DumpEntry(ses,el);
		if		(  ent->curr  ==  ent->head  ) {
			OBJ_POINT_NODE(el->prio);
		}
		ent->curr = OBJ_POS(ses,page,data->use);
		ent->node = ent->curr;
		ent->buff = (byte *)el + sizeof(OsekiDataEntry);
		ent->bsize = PAGE_SIZE(ses) -
			( data->use + sizeof(OsekiDataEntry));
		break;
	  case	OSEKI_ALLOC_SHORT:
		dbgmsg("*");
		ent->bsize = 0;
		break;
	}
	ent->use = 0;
LEAVE_FUNC;
}

static	ssize_t
WriteEntry(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	byte			*buff,
	size_t			size)
{
	ssize_t		ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	ret = 0;
	while	(  size  >  0  ) {
		left = ent->bsize - ent->use;
printf("left = %d\n",left);
		if		(  left  ==  0  ) {
			ReadBuffer(ses,ent,TRUE);
printf("bsize = %d\n",ent->bsize);
			if		(  ent->bsize  ==  0  ) {
				NewBuffer(ses,ent);
			}
printf("bsize = %d\n",ent->bsize);
			left = ent->bsize;
		}
		csize = ( size < left ) ? size : left;
printf("csize = %d\n",csize);
		memcpy(&ent->buff[ent->use],buff,csize);
		ent->fDurty = TRUE;
		ent->use += csize;
		if		(  ent->use  ==  ent->bsize  ) {
			FlushBuffer(ses,ent,FALSE);
		}
		ent->off += csize;
		ret += csize;
		size -= csize;
		buff += csize;
	}
printf("ent->off  = %lld\n",ent->off);
printf("ent->size = %lld\n",ent->size);
	if		(  ent->off  >  ent->size  ) {
		ent->size = ent->off;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ssize_t
ReadEntry(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	byte			*buff,
	size_t			size)
{
	ssize_t		ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	ret = 0;
	DumpInfo(ses,ent);
	FlushBuffer(ses,ent,TRUE);
	DumpInfo(ses,ent);
printf("ent->off  = %lld\n",ent->off);
printf("ent->size = %lld\n",ent->size);
	while	(	(  size      >  0          )
			&&	(  ent->off  <  ent->size  ) ) {
		left = ent->bsize - ent->use;
printf("left = %d\n",left);
		if		(  left  ==  0  ) {
			ReadBuffer(ses,ent,FALSE);
printf("bsize = %d\n",ent->bsize);
			if		(  ent->bsize  ==  0  )	break;
			left = ( ent->bsize > ( ent->size - ent->off ) ) ?
				( ent->size - ent->off ) : ent->bsize;
		}
		csize = ( size < left ) ? size : left;
		if		(  csize  ==  0  )	break;
printf("csize = %d\n",csize);
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
NewObject(
	OsekiSession	*ses,
	int				mode)
{
	ObjectType	obj;
	pageno_t	leaf;

ENTER_FUNC;
	ses->objs = GetPage(ses,1LL);
	obj = GetFreeOID(ses);
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		if		(  OpenEntry(ses,leaf,obj,(byte)mode|OSEKI_OPEN_CREATE)
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
	OsekiSession	*ses,
	ObjectType	obj,
	int				mode)
{
	pageno_t	leaf;
	ssize_t		ret;
	ObjectInfo	*ent;

ENTER_FUNC;
	ses->objs = GetPage(ses,1LL);
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		if		(  ( ent = OpenEntry(ses,leaf,obj,(byte)mode) )
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

extern	Bool
CloseObject(
	OsekiSession	*ses,
	ObjectType	obj)
{
	Bool			ret;
	ObjectInfo		*ent;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	ret = TRUE;
	Lock(&ses->space->obj);
	if		(  ( ent = g_hash_table_lookup(ses->oTable,&obj) )  !=  NULL  ) {
		CloseEntry(ses,ent,obj);
		g_hash_table_remove(ses->oTable,&obj);
		xfree(ent);
	} else {
		ret = FALSE;
	}
	UnLock(&ses->space->obj);
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
DestroyObject(
	OsekiSession	*ses,
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
	ObjectInfo		*ent;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	rc = TRUE;
	Lock(&ses->space->obj);
	page = SearchLeafPage(ses,obj,stack);
	if		(  page  ==  0  ) {
		rc = FALSE;
	} else {
		(void)GetPage(ses,page);
		leafpage = UpdatePage(ses,page);
		off = obj % LEAF_ELEMENTS(ses);
		FreeEntry(ses,&leafpage[off]);
		FREE_OBJ(leafpage[off]);
		own = page;
		for	( i = 0 ; i < ses->objs->level ; i ++ ) {
			page = stack[i];
			(void)GetPage(ses,page);
			nodepage = UpdatePage(ses,page);
			for	( j = 0 ; j < NODE_ELEMENTS(ses) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					FREE_NODE(nodepage[j]);
					break;
				}
			}
			own = page;
			if			(  PAGE_NO(ses->objs->pos[i])  ==  page  ) {
				FREE_NODE(ses->objs->pos[i]);
			}
			if		(  HAVE_FREECHILD(page)  )	break;
		}
		if		(  ( ent = g_hash_table_lookup(ses->oTable,&obj) )
				   !=  NULL  ) {
			g_hash_table_remove(ses->oTable,&obj);
			xfree(ent);
		}
		rc = TRUE;
	}
	UnLock(&ses->space->obj);
LEAVE_FUNC;
	return	(rc);
}

extern	int
WriteObject(
	OsekiSession	*ses,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	ssize_t		ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(ses->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = WriteEntry(ses,ent,buff,size);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	int
ReadObject(
	OsekiSession	*ses,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	int			ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(ses->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = ReadEntry(ses,ent,buff,size);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ssize_t
SeekEntry(
	OsekiSession	*ses,
	ObjectInfo		*ent,
	uint64_t		pos)
{
	ssize_t		ret;

	ret = 0;
	FlushBuffer(ses,ent,TRUE);
	switch	(ent->mode & OSEKI_TYPE_MASK) {
	  case	OSEKI_ALLOC_LINER:
		ent->curr = ent->head + pos;
		ent->off = pos;
		ent->use = OBJ_OFF(ses,pos);
		ReadBuffer(ses,ent,TRUE);
		break;
	  case	OSEKI_ALLOC_TREE:
		ent->off = pos;
		ReadBuffer(ses,ent,TRUE);
		break;
	  case	OSEKI_ALLOC_PACK:
		ent->curr = ent->head;
		ent->off = 0;
		ReadBuffer(ses,ent,TRUE);
		while	(	(  ent->off  <  pos        )
				&&	(  ent->off  <  ent->size  ) ) {
			ReadBuffer(ses,ent,FALSE);
			if		(  ent->bsize  ==  0  )	break;
			if		(  ent->off + ent->bsize >  pos  ) {
				ent->use = pos - ent->off;
				ent->off = pos;
			} else {
				ent->off += ent->bsize;
			}
		}
		break;
	  case	OSEKI_ALLOC_SHORT:
		if		(  pos  <  SIZE_SMALL  ) {
			ent->off = pos;
		}
		break;
	}
	DumpInfo(ses,ent);
	return	(ret);
}

extern	int
SeekObject(
	OsekiSession	*ses,
	ObjectType		obj,
	objpos_t		pos)
{
	ObjectInfo	*ent;
	int			ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(ses->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = SeekEntry(ses,ent,pos);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	ObjectBody	*
NewObjectBody(
	size_t	size,
	byte	*body)
{
	ObjectBody	*ret;

	if		( ( ret = (ObjectBody *)xmalloc(sizeof(ObjectBody) + size) )  !=  NULL  ) {
		ret->size = size;
		memcpy(ret->body,body,size);
	}
	return	(ret);
}

extern	void
DestroyObjectBody(
	ObjectBody	*obj)
{
	xfree(obj);
}

extern	ObjectType
InitiateObject(
	OsekiSession	*ses,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	ObjectType	obj;
	pageno_t	leaf;
	byte		mode;

ENTER_FUNC;
	ses->objs = GetPage(ses,1LL);
	obj = GetFreeOID(ses);
	mode = OSEKI_OPEN_WRITE | OSEKI_OPEN_CREATE;
	if		(  size  <=  SIZE_SMALL  ) {
		mode |= OSEKI_ALLOC_SHORT;
	}
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		if		(  ( ent = OpenEntry(ses,leaf,obj,
									 mode) )  ==  NULL  ) {
			obj = GL_OBJ_NULL;
		}
	} else {
		ent  = NULL;
		obj = GL_OBJ_NULL;
	}
	if		(  obj  !=  GL_OBJ_NULL  ) {
		WriteEntry(ses,ent,buff,size);
		CloseEntry(ses,ent,obj);
		g_hash_table_remove(ses->oTable,&obj);
		xfree(ent);
	}
LEAVE_FUNC;
	return	(obj);
}

extern	ObjectBody	*
GetObject(
	OsekiSession	*ses,
	ObjectType		obj)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	ObjectBody	*cont;

ENTER_FUNC;
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		ent = OpenEntry(ses,leaf,obj,OSEKI_OPEN_READ);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		cont = (ObjectBody *)xmalloc(sizeof(ObjectBody) + ent->size);
		cont->size = ent->size;
		ReadEntry(ses,ent,cont->body,cont->size);
		CloseEntry(ses,ent,obj);
		g_hash_table_remove(ses->oTable,&obj);
		xfree(ent);
	} else {
		cont = NULL;
	}
LEAVE_FUNC;
	return	(cont);
}

extern	ssize_t
GetObjectCopy(
	OsekiSession	*ses,
	ObjectType		obj,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	ssize_t		ret;

ENTER_FUNC;
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		ent = OpenEntry(ses,leaf,obj,OSEKI_OPEN_READ);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		if		(  size  ==  0  ) {
			size = ent->size;
		} else {
			size = ( size  <  ent->size ) ? size : ent->size;
		}
		ReadEntry(ses,ent,buff,size);
		CloseEntry(ses,ent,obj);
		g_hash_table_remove(ses->oTable,&obj);
		xfree(ent);
		ret = size;
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
PutObject(
	OsekiSession	*ses,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	byte		mode;

ENTER_FUNC;
	mode = OSEKI_OPEN_WRITE | OSEKI_OPEN_CREATE;
	if		(  size  <=  SIZE_SMALL  ) {
		mode |= OSEKI_ALLOC_SHORT;
	}
	if		(  ( leaf = SearchLeafPage(ses,obj,NULL) )  !=  0  ) {
		ent = OpenEntry(ses,leaf,obj,mode);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		WriteEntry(ses,ent,buff,size);
		CloseEntry(ses,ent,obj);
		g_hash_table_remove(ses->oTable,&obj);
		xfree(ent);
	}
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionStart(
	OsekiSession	*ses)
{
	int		tid;

ENTER_FUNC;
	Lock(ses->space);
	tid = ses->space->cTran ++;
	ses->tid = tid;
	g_tree_insert(ses->space->trans,&ses->tid,ses);
	UnLock(ses->space);
	ses->oTable = NewLLHash();
	ses->pages = NewLLHash();
	ses->objs = NULL;
LEAVE_FUNC;
	return	(TRUE);
}

static	void
SyncObject(
	ObjectType		*obj,
	ObjectInfo		*ent,
	OsekiSession	*ses)
{
ENTER_FUNC;
	CloseEntry(ses,ent,ent->obj);
	xfree(ent);
LEAVE_FUNC;
}

static	void
CommitObject(
	OsekiSession	*ses)
{
ENTER_FUNC;
	g_hash_table_foreach(ses->oTable,(GHFunc)SyncObject,ses);
	g_hash_table_destroy(ses->oTable);
	ses->oTable = NULL;
LEAVE_FUNC;
}

static	void
SetLeast(
	OsekiSession	*ses)
{
	int		_least(
		int				tid,
		OsekiSession	*ses,
		OsekiSpace		*space)
	{
		space->lTran = ses->tid;
		dbgprintf("tid = %d\n",space->lTran);
		return	(TRUE);
	}
ENTER_FUNC;
	g_tree_remove(ses->space->trans,(gpointer)ses->tid);
	g_tree_traverse(ses->space->trans,(GTraverseFunc)_least,
					G_IN_ORDER,ses->space);
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionCommit(
	OsekiSession	*ses)
{
ENTER_FUNC;
	SetLeast(ses);
	CommitObject(ses);
	CommitPages(ses);
	ses->tid = 0;
LEAVE_FUNC;
	return	(TRUE);
}

static	void
FreeObject(
	ObjectType		*obj,
	ObjectInfo		*ent,
	OsekiSession	*ses)
{
ENTER_FUNC;
	xfree(ent);
LEAVE_FUNC;
}

static	void
AbortObject(
	OsekiSession	*ses)
{
ENTER_FUNC;
	g_hash_table_foreach(ses->oTable,(GHFunc)FreeObject,ses);
	g_hash_table_destroy(ses->oTable);
	ses->oTable = NULL;
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionAbort(
	OsekiSession	*ses)
{
ENTER_FUNC;
	SetLeast(ses);
	AbortObject(ses);
	AbortPages(ses);
	ses->tid = 0;
LEAVE_FUNC;
	return	(TRUE);
}


