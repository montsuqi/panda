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
	OsekiSession	*ses)
{
	void	*ret;

	ret = xmalloc(ses->space->pagesize);
	memclear(ret,PAGE_SIZE(ses));

	return	(ret);
}

extern	void
_DumpPage(
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

#ifdef	DEBUG
//#define	DumpPage(s,d)		_DumpPage((s),(byte *)(d))
#define	DumpPage(s,d)		/*	*/
#else
#define	DumpPage(s,d)		/*	*/
#endif


extern	void
ReleasePageBuffer(
	void	*buff)
{
	xfree(buff);
}

extern	void	*
ReadPage(
	OsekiSession	*ses,
	pageno_t	page)
{
	void	*ret;

ENTER_FUNC;
	if		(  ( ret = NewPageBuffer(ses) )  !=  NULL  ) {
		fseek(ses->space->fp,page*ses->space->pagesize,SEEK_SET);
		fread(ret,ses->space->pagesize,1,ses->space->fp);
	} else {
		fprintf(stderr,"memory empty\n");
	}
LEAVE_FUNC;
	return	(ret);
}
	
extern	void
WritePage(
	OsekiSession	*ses,
	void		*buff,
	pageno_t	page)
{
	fseek(ses->space->fp,page*ses->space->pagesize,SEEK_SET);
	fwrite(buff,ses->space->pagesize,1,ses->space->fp);
	fflush(ses->space->fp);
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
	OsekiSession	*ses,
	pageno_t		n)
{
	pageno_t	page
		,		ret;
	void		*data;

ENTER_FUNC;
	if		(  n  ==  0  ) {
		ret = 0LL;
	} else {
		ret = ses->space->upages;
		for	( ; n > 0 ; n -- ) {
			Lock(ses->space);
			page = ses->space->upages;
			ses->space->upages ++;
			UnLock(ses->space);
			data = NewPageBuffer(ses);
			RegistCommonPage(ses->space,data,page);
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
	OsekiSession	*ses,
	pageno_t	page)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;
	void			*ret;

ENTER_FUNC;
	space = ses->space;
	if		(  ( cb = SearchCommonBuffer(space,page) )
			   ==  NULL  ) {
		ret = ReadPage(ses,page);
		cb = RegistCommonPage(space,ret,page);
	} else {
		if		(  cb->utid  >  0  ) {
			if		(  ses->tid  ==  cb->utid  ) {
				ret = cb->update;
			} else {
				if		(  cb->ltid  >  0  ) {
					if		(  ses->tid  <  cb->ltid  ) {
						ret = cb->old;
					} else {
						cb->current = ReadPage(ses,page);
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
	OsekiSession	*ses,
	pageno_t		page)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;
	void			*ret;

ENTER_FUNC;
	space = ses->space;
	if		(  ( cb = SearchCommonBuffer(space,page) )
			   !=  NULL  ) {
		Lock(cb);
		if		(  cb->update  ==  NULL  ) {
			cb->old = cb->current;
			cb->current = NULL;
			cb->update = NewPageBuffer(ses);
			memcpy(cb->update,cb->old,space->pagesize);
			cb->utid = ses->tid;
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
	OsekiSession	*ses,
	pageno_t		page,
	Bool			fCommit)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;

ENTER_FUNC;
	dbgprintf("sync    = [%lld]\n",page);

	space = ses->space;
	if		(  ( cb = SearchCommonBuffer(space,page) )
			   !=  NULL  ) {
		Lock(cb);
		if		(  fCommit  ) {
			WritePage(ses,cb->update,cb->page);
			cb->current = cb->update;
			cb->update = NULL;
			cb->ltid = space->cTran;
		}
		if		(  ses->space->lTran  >=  cb->ltid  ) {
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
	OsekiSession	*ses,
	pageno_t		page)
{
	PageInfo	*ent;
	void		*ret;

ENTER_FUNC;
	dbgprintf("get page = [%lld]\n",page);
	dbgprintf("pages    = %lld\n",ses->space->upages);
	if		(  page  <  ses->space->upages  ) {
		if		(  ( ent = (PageInfo *)g_hash_table_lookup(ses->pages,&page) )
				   ==  NULL  ) {
			ent = New(PageInfo);
			ent->fUpdate = FALSE;
			ent->page = page;
			ent->body = GetCommonPage(ses,page);
			g_hash_table_insert(ses->pages,&ent->page,ent);
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
	OsekiSession	*ses,
	pageno_t		page)
{
	void		*ret;
	PageInfo	*ent;

ENTER_FUNC;
	if		(  ( ent = (PageInfo *)g_hash_table_lookup(ses->pages,&page) )
			   !=  NULL  ) {
		if		(  !ent->fUpdate  ) {
			ent->fUpdate = TRUE;
			ret = UpdateCommonPage(ses,page);
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
	OsekiSession	*ses,
	pageno_t		page,
	Bool			fCommit)
{
	PageInfo	*ent;

ENTER_FUNC;
	dbgprintf("release = [%lld]\n",page);
	if		(  page  ==  0  ) {
	} else
	if		(  ( ent = (PageInfo *)g_hash_table_lookup(ses->pages,&page) )
			   !=  NULL  ) {
		SyncCommonPage(ses,page,(fCommit && ent->fUpdate));
		xfree(ent);
	}
LEAVE_FUNC;
}

extern	pageno_t
GetFreePage(
	OsekiSession	*ses)
{
	OsekiSpace	*space;
	pageno_t	no;
	int			i;

ENTER_FUNC;
	space = ses->space;
	(void)GetPage(ses,ses->objs->freedata);
	space->freedata = UpdatePage(ses,ses->objs->freedata);
	no = 0;
#ifdef	DEBUG
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  !=  0  ) {
			printf("free page = %lld\n",space->freedata[i]);
		}
	}
#endif
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  !=  0  ) {
			no = space->freedata[i];
			space->freedata[i] = 0;
			break;
		}
	}
	if		(  no  ==  0  ) {
		no = NewPage(ses,1);
	}
	dbgprintf("free page = %lld\n",no);
LEAVE_FUNC;
	return	(no);
}

extern	void
ReturnPage(
	OsekiSession	*ses,
	pageno_t		no)
{
	OsekiSpace	*space;
	int				i;

ENTER_FUNC;
dbgprintf("return = [%lld]\n",no);
	space = ses->space;
	(void)GetPage(ses,ses->objs->freedata);
	space->freedata = UpdatePage(ses,ses->objs->freedata);
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  ==  no  )	break;
		if		(  space->freedata[i]  ==  0  ) {
			space->freedata[i] = no;
			break;
		}
	}
#ifdef	DEBUG
	for	( i = 0 ; i < space->pagesize / sizeof(pageno_t) ; i ++ ) {
		if		(  space->freedata[i]  !=  0  ) {
			printf("free page = %lld\n",space->freedata[i]);
		}
	}
#endif
LEAVE_FUNC;
}

static	void
SaveOld(
	pageno_t		*page,
	PageInfo		*ent,
	OsekiSession	*ses)
{
	OsekiSpace		*space;
	CommonBuffer	*cb;

ENTER_FUNC;
	space = ses->space;
	if		(  ( cb = SearchCommonBuffer(space,*page) )
			   !=  NULL  ) {
		Lock(cb);
		if		(  cb->old  !=  NULL  ) {
			fwrite(page,sizeof(pageno_t),1,space->fp);
			fwrite(cb->old,space->pagesize,1,space->fp);
			ses->cOld ++;
		}
		UnLock(cb);
	}
LEAVE_FUNC;
}

static	void
CommitPage(
	pageno_t		*page,
	PageInfo		*ent,
	OsekiSession	*ses)
{
ENTER_FUNC;
	ReleasePage(ses,*page,TRUE);
LEAVE_FUNC;
}

static	void
AbortPage(
	pageno_t		*page,
	PageInfo		*ent,
	OsekiSession	*ses)
{
ENTER_FUNC;
	ReleasePage(ses,*page,FALSE);
LEAVE_FUNC;
}

extern	void
CommitPages(
	OsekiSession	*ses)
{
	OsekiSpace		*space;
	OsekiHeaderPage	*head;

ENTER_FUNC;
	space = ses->space;
	head = ReadPage(ses,0LL);
	Lock(space);
	fseek(space->fp,space->upages*space->pagesize,SEEK_SET);
	ses->cOld = 0;
	g_hash_table_foreach(ses->pages,(GHFunc)SaveOld,ses);
	fflush(space->fp);
	if		(  ses->cOld  >  0  ) {
		memcpy(&head->phase[ALTER_GENERATION(space)],
			   &head->phase[CURRENT_GENERATION(space)],
			   sizeof(OsekiPhaseControl));
		head->phase[ALTER_GENERATION(space)].seq = space->cSeq ++;
		head->phase[ALTER_GENERATION(space)].cOld = ses->cOld;
		head->phase[ALTER_GENERATION(space)].pages = space->upages;
		WritePage(ses,head,0LL);
		g_hash_table_foreach(ses->pages,(GHFunc)CommitPage,ses);
		head->phase[ALTER_GENERATION(space)].cOld = 0;
		WritePage(ses,head,0LL);
		CURRENT_GENERATION(space) = ALTER_GENERATION(space);
	} else {
		g_hash_table_foreach(ses->pages,(GHFunc)AbortPage,ses);
	}
	ReleasePageBuffer(head);
	g_hash_table_destroy(ses->pages);
	UnLock(space);
	ses->pages = NULL;
LEAVE_FUNC;
}

extern	void
AbortPages(
	OsekiSession	*ses)
{
	g_hash_table_foreach(ses->pages,(GHFunc)AbortPage,ses);
	g_hash_table_destroy(ses->pages);
	ses->pages = NULL;
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
	pageno_t		cp
		,			page;
	int64_t			mul;
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
		space->root = head.root;
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
				old = NewPageBuffer(recover);
				fread(old,space->pagesize,1,space->fp);
				(void)RegistCommonPage(space,old,page);
				GetPage(recover,page);
				UpdatePage(recover,page);
				CommitPages(recover);
			}
			DisConnectOseki(recover);
		}
		//InitializeModules(space);
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
	OsekiSession	*ses)
{
ENTER_FUNC;
	if		(  ses->pages  !=  NULL  ) {
		AbortPages(ses);
	}
	xfree(ses);
LEAVE_FUNC;
}

