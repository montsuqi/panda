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
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/mman.h>

#include	"types.h"
#include	"libmondai.h"
#include	"lock.h"
#include	"page.h"
#include	"pagestruct.h"
#include	"table.h"
#include	"message.h"
#include	"debug.h"

typedef	struct {
	pageno_t	page;
	Bool		fUpdate;
	void		*body;
}	PageInfo;

typedef struct	{
	pageno_t	page;
	void		*old;
	void		*update;
	void		*current;
	int			utid;
	int			ltid;
	int			cRef;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	CommonBuffer;

static	void	*
NewPageBuffer(
	OsekiSpace	*space)
{
	void	*ret;

	ret = xmalloc(space->pagesize);

	return	(ret);
}

extern	pageno_t
NewPage(
	OsekiSession	*state)
{
	CommonBuffer	*cb;
	pageno_t	page;

ENTER_FUNC;
	Lock(state->space);
	page = state->space->upages;
	state->space->upages ++;
	cb = New(CommonBuffer);
	cb->page = page;
	cb->old = NULL;
	cb->update = NULL;
	cb->current = NewPageBuffer(state->space);
	memclear(cb->current,state->space->pagesize);
	cb->utid = 0;
	cb->ltid = 0;
	cb->cRef = 0;
	InitLock(cb);
	g_hash_table_insert(state->space->pages,&cb->page,cb);
	UnLock(state->space);
	dbgprintf("new page = %lld\n",page);
LEAVE_FUNC;
	return	(page);
}

static	void	*
ReadPage(
	OsekiSpace	*space,
	pageno_t	page)
{
	void	*ret;

ENTER_FUNC;
	if		(  ( ret = NewPageBuffer(space) )  !=  NULL  ) {
		fseek(space->fp,page*space->pagesize,SEEK_SET);
		fread(ret,space->pagesize,1,space->fp);
	} else {
		fprintf(stderr,"memory empty\n");
	}
LEAVE_FUNC;
	return	(ret);
}
	
static	void
WritePage(
	OsekiSpace	*space,
	void		*buff,
	pageno_t	page)
{
	fseek(space->fp,page*space->pagesize,SEEK_SET);
	fwrite(buff,space->pagesize,1,space->fp);
	fflush(space->fp);
}

static	CommonBuffer	*
SearchCommonBuffer(
	OsekiSpace	*space,
	pageno_t	page)
{
	CommonBuffer	*cb;

ENTER_FUNC;
	Lock(space);
	cb = (CommonBuffer *)g_hash_table_lookup(space->pages,&page);
	UnLock(space);
LEAVE_FUNC;
	return	(cb);
}

static	void	*
GetCommonPage(
	OsekiSession	*state,
	pageno_t	page)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;
	void			*ret;

ENTER_FUNC;
	space = state->space;
	if		(  ( cb = SearchCommonBuffer(space,page) )
			   ==  NULL  ) {
		cb = New(CommonBuffer);
		cb->page = page;
		cb->old = NULL;
		cb->update = NULL;
		cb->current = ReadPage(space,page);
		cb->utid = 0;
		cb->ltid = 0;
		cb->cRef = 0;
		InitLock(cb);
		Lock(space);
		g_hash_table_insert(space->pages,&cb->page,cb);
		UnLock(space);
		ret = cb->current;
	} else {
		if		(  cb->utid  >  0  ) {
			if		(  state->tid  ==  cb->utid  ) {
				ret = cb->update;
			} else {
				if		(  cb->ltid  >  0  ) {
					if		(  state->tid  <  cb->ltid  ) {
						ret = cb->old;
					} else {
						cb->current = ReadPage(space,page);
						ret = cb->current;
					}
				} else {
					ret = cb->current;
				}
			}
		} else {
			ret = cb->current;
		}
	}
	cb->cRef ++;
LEAVE_FUNC;
	return	(ret);
}

static	void	*
UpdateCommonPage(
	OsekiSession	*state,
	pageno_t		page)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;
	void			*ret;

ENTER_FUNC;
	space = state->space;
	if		(  ( cb = SearchCommonBuffer(space,page) )
			   !=  NULL  ) {
		Lock(cb);
		if		(  cb->update  ==  NULL  ) {
			cb->old = cb->current;
			cb->current = NULL;
			cb->update = NewPageBuffer(space);
			memcpy(cb->update,cb->old,space->pagesize);
			cb->utid = state->tid;
			ret = cb->update;
		} else {
			ret = NULL;
		}
		UnLock(cb);
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
SyncCommonPage(
	OsekiSession	*state,
	pageno_t		page,
	Bool			fCommit)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;

ENTER_FUNC;
	dbgprintf("sync    = [%lld]\n",page);

	space = state->space;
	if		(  ( cb = SearchCommonBuffer(space,page) )
			   !=  NULL  ) {
		Lock(cb);
		if		(  fCommit  ) {
			WritePage(space,cb->update,cb->page);
			cb->current = cb->update;
			cb->update = NULL;
			cb->ltid = space->cTran;
		}
		if		(  state->space->lTran  >=  cb->ltid  ) {
			xfree(cb->old);
			cb->old = NULL;
		}
		cb->cRef --;
		if		(	(  cb->old   ==  NULL  )
				&&	(  cb->cRef  ==  0     ) ) {
			Lock(space);
			UnLock(cb);
			DestroyLock(cb);
			if		(  cb->current  !=  NULL  ) {
				xfree(cb->current);
			}
			xfree(cb);
			g_hash_table_remove(space->pages,&page);
			UnLock(space);
		} else {
			UnLock(cb);
		}
	}
LEAVE_FUNC;
}

extern	void	*
GetPage(
	OsekiSession	*state,
	pageno_t		page)
{
	PageInfo	*ent;
	void		*ret;

ENTER_FUNC;
	dbgprintf("get page = %lld\n",page);
	if		(  page  <  state->space->upages  ) {
		if		(  ( ent = (PageInfo *)g_hash_table_lookup(state->pages,&page) )
				   ==  NULL  ) {
			ent = New(PageInfo);
			ent->fUpdate = FALSE;
			ent->page = page;
			ent->body = GetCommonPage(state,page);
			g_hash_table_insert(state->pages,&ent->page,ent);
		}
		ret = ent->body;
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void	*
UpdatePage(
	OsekiSession	*state,
	pageno_t		page)
{
	void		*ret;
	PageInfo	*ent;

ENTER_FUNC;
	if		(  ( ent = (PageInfo *)g_hash_table_lookup(state->pages,&page) )
			   !=  NULL  ) {
		if		(  !ent->fUpdate  ) {
			ent->fUpdate = TRUE;
			ret = UpdateCommonPage(state,page);
			ent->body = ret;
		} else {
			ret = ent->body;
		}
	} else {
		fprintf(stderr,"fatal\n");
		exit(1);
	}
LEAVE_FUNC;
	return	(ret);
}


extern	void
ReleasePage(
	OsekiSession	*state,
	pageno_t		page,
	Bool			fCommit)
{
	PageInfo	*ent;

ENTER_FUNC;
	dbgprintf("release = [%lld]\n",page);
	if		(  ( ent = (PageInfo *)g_hash_table_lookup(state->pages,&page) )
			   !=  NULL  ) {
		SyncCommonPage(state,page,(fCommit && ent->fUpdate));
		xfree(ent);
	}
LEAVE_FUNC;
}

extern	pageno_t
GetFreePage(
	OsekiSession	*state)
{
	OsekiSpace	*space;
	pageno_t	no;
	int			i;

ENTER_FUNC;
	space = state->space;
	no = 0;
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  !=  0  ) {
			no = space->freedata[i];
			space->freedata[i] = 0;
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

extern	void
ReturnPage(
	OsekiSession	*state,
	pageno_t		no)
{
	OsekiSpace	*space;
	int				i;

ENTER_FUNC;
	space = state->space;
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  ==  0  ) {
			space->freedata[i] = no;
			break;
		}
	}
	WritePage(state->space,state->space->freedata,state->space->freedatapage);
LEAVE_FUNC;
}

static	void
CommitPage(
	pageno_t		*page,
	PageInfo		*ent,
	OsekiSession	*state)
{
ENTER_FUNC;
	ReleasePage(state,*page,TRUE);
LEAVE_FUNC;
}

extern	void
CommitPages(
	OsekiSession	*state)
{
ENTER_FUNC;
	g_hash_table_foreach(state->pages,(GHFunc)CommitPage,state);
	g_hash_table_destroy(state->pages);
	state->pages = NULL;
LEAVE_FUNC;
}

static	void
AbortPage(
	pageno_t		*page,
	PageInfo		*ent,
	OsekiSession	*state)
{
ENTER_FUNC;
	ReleasePage(state,*page,FALSE);
LEAVE_FUNC;
}

extern	void
AbortPages(
	OsekiSession	*state)
{
	g_hash_table_foreach(state->pages,(GHFunc)AbortPage,state);
	g_hash_table_destroy(state->pages);
	state->pages = NULL;
}

extern	OsekiSpace	*
InitOseki(
	char	*file)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	char	buff[SIZE_BUFF];
	OsekiSpace		*space;
	OsekiFileHeader	head;
	int				i;
	pageno_t		mul;

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s.pid",file);
	if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
		if		(  fgets(buff,SIZE_BUFF,fp)  !=  NULL  ) {
			int pid = atoi(buff);
			if		(	(  getpid() != pid  )
					&&	(  kill(pid, 0) == 0 || errno == EPERM  ) ) {
				fprintf(	stderr,"another process uses libpandablob. (%d)\n",atoi(buff));
				exit(1);
			}
		}
		fclose(fp);
	} else {
		if		(  ( fp = Fopen(name,"w") )  !=  NULL  ) {
			fprintf(fp,"%d\n",getpid());
			fclose(fp);
		} else {
			fprintf(stderr,"can not make lock file(directory not writable?)\n");
			exit(1);
		}
	}

	snprintf(name,SIZE_LONGNAME+1,"%s.pnb",file);
	if		(  ( fp = fopen(name,"r+") )  !=  NULL  ) {
		fread(&head,sizeof(head),1,fp);
		if		(  memcmp(head.magic,OSEKI_FILE_HEADER,OSEKI_MAGIC_SIZE)  !=  0  ) {
			fprintf(stderr,"version mismatch\n");
			exit(1);
		}
		space = New(OsekiSpace);
		space->file = StrDup(file);
		space->fp = fp;
		space->pagesize = head.pagesize;
		mul = 1;
		for	( i = 0 ; i < MAX_PAGE_LEVEL ; i ++ ) {
			space->mul[i] = mul;
			mul *= head.pagesize / sizeof(pageno_t);
		}
		for	( i = 0 ; i < MAX_PAGE_LEVEL ; i ++ ) {
			space->pos[i] = head.pos[i];
		}
		space->upages = head.pages;
		space->level = head.level;
		space->freedata = ReadPage(space,head.freedata);
		space->freedatapage = head.freedata;
		space->freepage = NewLLHash();
		space->pages = NewLLHash();
		space->trans = NewIntTree();
		space->cTran = 1;
		space->lTran = 0;
		InitLock(space);
	} else {
		space = NULL;
	}

LEAVE_FUNC;
	return	(space);
}

extern	void
FinishOseki(
	OsekiSpace	*space)
{
	char	name[SIZE_LONGNAME+1];
	OsekiFileHeader	head;
	int		i;

ENTER_FUNC;
	WritePage(space,space->freedata,space->freedatapage);
	snprintf(name,SIZE_LONGNAME+1,"%s.pid",space->file);
	unlink(name);
	memcpy(head.magic,OSEKI_FILE_HEADER,OSEKI_MAGIC_SIZE);
	head.freedata = space->freedatapage;
	head.pagesize = space->pagesize;
	head.pages = space->upages;
	head.level = space->level;
	for	( i = 0 ; i < MAX_PAGE_LEVEL ; i ++ ) {
		head.pos[i] = space->pos[i];
	}
	rewind(space->fp);
	fwrite(&head,sizeof(head),1,space->fp);
	fclose(space->fp);

	xfree(space->file);
	DestroyLock(space);
	xfree(space);
LEAVE_FUNC;
}

extern	OsekiSession	*
ConnectOseki(
	OsekiSpace	*space)
{
	OsekiSession	*ret;

ENTER_FUNC;
	ret = New(OsekiSession);
	ret->space = space;
	ret->tid = 0;
	ret->pages = NULL;
LEAVE_FUNC;
	return	(ret);
}

extern	void
DisConnectOseki(
	OsekiSession	*state)
{
ENTER_FUNC;
	if		(  state->pages  !=  NULL  ) {
		g_hash_table_destroy(state->pages);
	}
	xfree(state);
LEAVE_FUNC;
}

