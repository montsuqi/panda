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
#include	<sys/mman.h>

#include	"types.h"
#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"
#include	"blob_v2.h"
#include	"message.h"
#include	"debug.h"

extern	pageno_t
NewPage(
	BLOB_V2_State	*state)
{
	pageno_t	page
		,		zero;
	int			i;

	page = state->space->pages;
	state->space->pages ++;
	fseek(state->space->fp,0,SEEK_END);
	zero = 0;
	for	( i = 0 ; i < NODE_ELEMENTS(state) ; i ++ ) {
		fwrite(&zero,sizeof(pageno_t),1,state->space->fp);
	}
	fflush(state->space->fp);
	dbgprintf("new page = %lld\n",page);
	return	(page);
}

extern	void	*
GetPage(
	BLOB_V2_State	*state,
	pageno_t		page)
{
	BLOB_V2_Page	*ent;

ENTER_FUNC;
	dbgprintf("get page = %lld\n",page);
	if		(  ( ent = (BLOB_V2_Page *)g_hash_table_lookup(state->pages,&page) )
			   ==  NULL  ) {
		ent = New(BLOB_V2_Page);
		ent->fUpdate = FALSE;
		ent->page = page;
		g_hash_table_insert(state->pages,&ent->page,ent);
#ifdef	USE_MMAP
		ent->body = mmap(NULL,state->space->pagesize,
						 PROT_READ|PROT_WRITE,MAP_SHARED,fileno(state->space->fp),
						 page*state->space->pagesize);
#else
		ent->body = xmalloc(state->space->pagesize);
		fseek(state->space->fp,page*state->space->pagesize,SEEK_SET);
		fread(ent->body,state->space->pagesize,1,state->space->fp);
#endif
	}
LEAVE_FUNC;
	return	(ent->body);
}

extern	void
UpdatePage(
	BLOB_V2_State	*state,
	pageno_t		page)
{
	BLOB_V2_Page	*ent;

ENTER_FUNC;
	if		(  ( ent = (BLOB_V2_Page *)g_hash_table_lookup(state->pages,&page) )
			   !=  NULL  ) {
		ent->fUpdate = TRUE;
	} else {
		dbgmsg("*");
	}
LEAVE_FUNC;
}

extern	void
ReleasePage(
	BLOB_V2_State	*state,
	pageno_t		page)
{
	BLOB_V2_Page	*ent;

ENTER_FUNC;
	dbgprintf("release = [%lld]\n",page);
	if		(  ( ent = (BLOB_V2_Page *)g_hash_table_lookup(state->pages,&page) )
			   !=  NULL  ) {
		if		(  ent->fUpdate  ) {
			dbgmsg("update");
#ifdef	USE_MMAP
			msync(ent->body,state->space->pagesize,MS_SYNC);
#else
			fseek(state->space->fp,page*state->space->pagesize,SEEK_SET);
			fwrite(ent->body,state->space->pagesize,1,state->space->fp);
			fflush(state->space->fp);
#endif
		}
#ifdef	USE_MMAP
		munmap(ent->body,state->space->pagesize);
#else
		xfree(ent->body);
#endif
		g_hash_table_remove(state->pages,&page);
		xfree(ent);
	}
LEAVE_FUNC;
}

extern	Bool
StartBLOB_V2(
	BLOB_V2_State	*state)
{
	int		tid;

	LockBLOB(state->space);
	tid = state->space->cTran ++;
	UnLockBLOB(state->space);
	ReleaseBLOB(state->space);
	state->tid = tid;
	state->oTable = NewLLHash();

	return	(TRUE);
}

static	void
SyncObject(
	MonObjectType	*obj,
	BLOB_V2_Open	*ent,
	BLOB_V2_State	*state)
{
	/*	free	*/
	xfree(ent->buff);
	xfree(ent);
}	

extern	Bool
CommitBLOB_V2(
	BLOB_V2_State	*state)
{
	state->tid = 0;

	g_hash_table_foreach(state->oTable,(GHFunc)SyncObject,state);
	g_hash_table_destroy(state->oTable);
	state->oTable = NULL;
	return	(TRUE);
}

extern	Bool
AbortBLOB_V2(
	BLOB_V2_State	*state)
{
	return	(TRUE);
}

extern	BLOB_V2_Space	*
InitBLOB_V2(
	char	*space)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	char	buff[SIZE_BUFF];
	BLOB_V2_Space	*blob;
	BLOB_V2_Header	head;
	int				i;
	pageno_t		mul;

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s.pid",space);
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

	snprintf(name,SIZE_LONGNAME+1,"%s.pnb",space);
	if		(  ( fp = fopen(name,"r+") )  !=  NULL  ) {
		fread(&head,sizeof(head),1,fp);
		if		(  memcmp(head.magic,BLOB_V2_HEADER,SIZE_BLOB_HEADER)  !=  0  ) {
			fprintf(stderr,"version mismatch\n");
			exit(1);
		}
		blob = New(BLOB_V2_Space);
		blob->space = StrDup(space);
		blob->fp = fp;
		blob->pagesize = head.pagesize;
		blob->pages = head.pages;
		blob->level = head.level;
		mul = 1;
		for	( i = 0 ; i < MAX_PAGE_LEVEL ; i ++ ) {
			blob->pos[i] = head.pos[i];
			blob->mul[i] = mul;
			mul *= head.pagesize / sizeof(pageno_t);
		}
		blob->cTran = 1;
		pthread_mutex_init(&blob->mutex,NULL);
		pthread_cond_init(&blob->cond,NULL);
	} else {
		blob = NULL;
	}

LEAVE_FUNC;
	return	(blob);
}

extern	void
FinishBLOB_V2(
	BLOB_V2_Space	*blob)
{
	char	name[SIZE_LONGNAME+1];
	BLOB_V2_Header	head;
	int		i;

ENTER_FUNC;
	snprintf(name,SIZE_LONGNAME+1,"%s.pid",blob->space);
	unlink(name);
	memcpy(head.magic,BLOB_V2_HEADER,SIZE_BLOB_HEADER);
	head.pagesize = blob->pagesize;
	head.pages = blob->pages;
	head.level = blob->level;
	for	( i = 0 ; i < MAX_PAGE_LEVEL ; i ++ ) {
		head.pos[i] = blob->pos[i];
	}
	rewind(blob->fp);
	fwrite(&head,sizeof(head),1,blob->fp);
	fclose(blob->fp);

	xfree(blob->space);
	pthread_mutex_destroy(&blob->mutex);
	pthread_cond_destroy(&blob->cond);
	xfree(blob);
LEAVE_FUNC;
}

extern	BLOB_V2_State	*
ConnectBLOB_V2(
	BLOB_V2_Space	*blob)
{
	BLOB_V2_State	*ret;

	ret = New(BLOB_V2_State);
	ret->space = blob;
	ret->tid = 0;
	ret->pages = NewLLHash();
	return	(ret);
}

extern	void
DisConnectBLOB_V2(
	BLOB_V2_State	*state)
{
	g_hash_table_destroy(state->pages);
	xfree(state);
}

