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
#include	<ctype.h>
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
#include	"storage.h"
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

#define	ALTER_GENERATION(s)		(((s)->inuse==0)?1:0)
#define	CURRENT_GENERATION(s)	((s)->inuse)

static	void	*
NewPageBuffer(
	OsekiSpace	*space)
{
	void	*ret;

	ret = xmalloc(space->pagesize);

	return	(ret);
}

extern	void
DumpPage(
	OsekiSpace	*space,
	byte		*data)
{
	int		i;

	for	( i = 0 ; i < space->pagesize ; i ++ , data ++) {
		if	(  isprint(*data)  ) {
			printf("%c",(int)*data);
		} else {
			printf("[%02X]",(int)*data);
		}
	}
	printf("\n");
}


static	void
ReleasePageBuffer(
	void	*buff)
{
	xfree(buff);
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
RegistCommonPage(
	OsekiSpace		*space,
	void		*data,
	pageno_t	page)
{
	CommonBuffer	*cb;

	cb = New(CommonBuffer);
	cb->page = page;
	cb->old = NULL;
	cb->update = NULL;
	cb->current = data;
	cb->utid = 0;
	cb->ltid = 0;
	cb->cRef = 0;
	InitLock(cb);
	Lock(space);
	g_hash_table_insert(space->pages,&cb->page,cb);
	UnLock(space);
	return	(cb);
}

extern	pageno_t
NewPage(
	OsekiSession	*state,
	pageno_t		n)
{
	pageno_t	page
		,		ret;
	void		*data;

ENTER_FUNC;
	if		(  n  ==  0  ) {
		ret = 0LL;
	} else {
		ret = state->space->upages;
		for	( ; n > 0 ; n -- ) {
			Lock(state->space);
			page = state->space->upages;
			state->space->upages ++;
			UnLock(state->space);
			data = NewPageBuffer(state->space);
			memclear(data,PAGE_SIZE(state));
			RegistCommonPage(state->space,data,page);
		}
	}
	dbgprintf("new page = %lld\n",ret);
LEAVE_FUNC;
	return	(ret);
}

static	CommonBuffer	*
SearchCommonBuffer(
	OsekiSpace	*space,
	pageno_t	page)
{
	CommonBuffer	*cb;

ENTER_FUNC;
	Lock(&space->page);
	cb = (CommonBuffer *)g_hash_table_lookup(space->pages,&page);
	UnLock(&space->page);
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
		ret = ReadPage(space,page);
		cb = RegistCommonPage(space,ret,page);
	} else {
		if		(  cb->utid  >  0  ) {
			if		(  state->tid  ==  cb->utid  ) {
dbgmsg("*");
				ret = cb->update;
			} else {
				if		(  cb->ltid  >  0  ) {
					if		(  state->tid  <  cb->ltid  ) {
dbgmsg("*");
						ret = cb->old;
					} else {
dbgmsg("*");
						cb->current = ReadPage(space,page);
						ret = cb->current;
					}
				} else {
dbgmsg("*");
					ret = cb->current;
				}
			}
		} else {
dbgmsg("*");
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
			Lock(&space->page);
			UnLock(cb);
			DestroyLock(cb);
			if		(  cb->current  !=  NULL  ) {
				xfree(cb->current);
			}
			xfree(cb);
			g_hash_table_remove(space->pages,&page);
			UnLock(&space->page);
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
	dbgprintf("pages    = %lld\n",state->space->upages);
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
		//DumpPage(state->space,ret);
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
	space->freedata = GetPage(state,state->objs->freedata);
	no = 0;
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  !=  0  ) {
			no = space->freedata[i];
			space->freedata = UpdatePage(state,state->objs->freedata);
			space->freedata[i] = 0;
			break;
		}
	}
	if		(  no  ==  0  ) {
		no = NewPage(state,1);
	}
	dbgprintf("free page = %lld\n",no);
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
dbgprintf("return = [%lld]\n",no);
	space = state->space;
	space->freedata = GetPage(state,state->objs->freedata);
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  ==  0  ) {
			space->freedata = UpdatePage(state,state->objs->freedata);
			space->freedata[i] = no;
			break;
		}
	}
LEAVE_FUNC;
}

static	void
SaveOld(
	pageno_t		*page,
	PageInfo		*ent,
	OsekiSession	*state)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;

ENTER_FUNC;
	space = state->space;
	if		(  ( cb = SearchCommonBuffer(space,*page) )
			   !=  NULL  ) {
		Lock(cb);
		if		(  cb->old  !=  NULL  ) {
			fwrite(page,sizeof(pageno_t),1,space->fp);
			fwrite(cb->old,space->pagesize,1,space->fp);
			state->cOld ++;
		}
		UnLock(cb);
	}
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
CommitPages(
	OsekiSession	*state)
{
	OsekiSpace		*space;
	OsekiHeaderPage	*head;

ENTER_FUNC;
	space = state->space;
	head = ReadPage(space,0LL);
	Lock(space);
	fseek(space->fp,space->upages*space->pagesize,SEEK_SET);
	state->cOld = 0;
	g_hash_table_foreach(state->pages,(GHFunc)SaveOld,state);
	fflush(space->fp);
	if		(  state->cOld  >  0  ) {
		memcpy(&head->phase[ALTER_GENERATION(space)],
			   &head->phase[CURRENT_GENERATION(space)],
			   sizeof(OsekiPhaseControl));
		head->phase[ALTER_GENERATION(space)].seq = space->cSeq ++;
		head->phase[ALTER_GENERATION(space)].cOld = state->cOld;
		head->phase[ALTER_GENERATION(space)].pages = space->upages;
		WritePage(space,head,0LL);
		g_hash_table_foreach(state->pages,(GHFunc)CommitPage,state);
		head->phase[ALTER_GENERATION(space)].cOld = 0;
		WritePage(space,head,0LL);
		CURRENT_GENERATION(space) = ALTER_GENERATION(space);
	} else {
		g_hash_table_foreach(state->pages,(GHFunc)AbortPage,state);
	}
	ReleasePageBuffer(head);
	g_hash_table_destroy(state->pages);
	UnLock(space);
	state->pages = NULL;
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
	OsekiHeaderPage	head;
	int				i;
	pageno_t		mul
		,			cp
		,			page;
	void			*old;
	OsekiSession	*recover;

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
		space->freedata = NULL;
		space->freepage = NewLLHash();
		space->pages = NewLLHash();
		space->trans = NewIntTree();
		space->cTran = 1;
		space->lTran = 0;
		space->pagesize = head.pagesize;
		mul = 1;
		for	( i = 0 ; i < MAX_PAGE_LEVEL ; i ++ ) {
			space->mul[i] = mul;
			mul *= head.pagesize / sizeof(pageno_t);
		}
		if		(  head.phase[0].seq  >  head.phase[1].seq  ) {
			space->inuse = 0;
		} else {
			space->inuse = 1;
		}
		InitLock(space);
		InitLock(&space->page);
		space->upages = head.phase[CURRENT_GENERATION(space)].pages;
		space->cSeq = head.phase[CURRENT_GENERATION(space)].seq + 1;
#ifdef	DEBUG
		printf("head.phase[0].pages = %lld\n",head.phase[0].pages);
		printf("head.phase[0].seq   = %d\n",head.phase[0].seq);
		printf("head.phase[0].cOld  = %lld\n",head.phase[0].cOld);
		printf("head.phase[1].pages = %lld\n",head.phase[1].pages);
		printf("head.phase[1].seq   = %d\n",head.phase[1].seq);
		printf("head.phase[1].cOld  = %lld\n",head.phase[1].cOld);
#endif
		if		(  head.phase[CURRENT_GENERATION(space)].cOld  >  0  ) {
			recover = ConnectOseki(space);
			OsekiTransactionStart(recover);
			CURRENT_GENERATION(space) = ALTER_GENERATION(space);
			space->upages = head.phase[CURRENT_GENERATION(space)].pages;
			space->cSeq = head.phase[CURRENT_GENERATION(space)].seq + 1;
			fseek(space->fp,space->upages*space->pagesize,SEEK_SET);
			for	( cp = 0 ; cp < head.phase[CURRENT_GENERATION(space)].cOld ; cp ++ ) {
				fread(&page,sizeof(pageno_t),1,space->fp);
				old = NewPageBuffer(space);
				fread(old,space->pagesize,1,space->fp);
				(void)RegistCommonPage(space,old,page);
				GetPage(recover,page);
				UpdatePage(recover,page);
				CommitPages(recover);
			}
			DisConnectOseki(recover);
		}
	} else {
		space = NULL;
	}
printf("%d\n",(int)space);fflush(stdout);

LEAVE_FUNC;
	return	(space);
}

extern	void
FinishOseki(
	OsekiSpace	*space)
{
	char	name[SIZE_LONGNAME+1];

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s.pid",space->file);
	unlink(name);
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
		AbortPages(state);
	}
	xfree(state);
LEAVE_FUNC;
}

