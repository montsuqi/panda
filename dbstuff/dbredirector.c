/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<sys/socket.h>
#include	<sys/select.h>
#include	<time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"comm.h"
#include	"dirs.h"
#include	"redirect.h"
#include	"dbgroup.h"
#include	"dblib.h"
#include	"directory.h"
#include	"queue.h"
#include	"socket.h"
#include	"dbredirector.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*PortNumber;
static	int		Back;
static	char	*Directory;

static	DBG_Struct	*ThisDBG;
static	pthread_t	_FileThread;

static	Port		*RedirectPort;
static  pthread_mutex_t redlock;
static  pthread_cond_t redcond;  

static 	Bool 	fShutdown = FALSE;

static  GSList *TicketList;
static  uint64_t TICKETID = 0;

static VeryfyData	*
NewVerfyData(void)
{
	VeryfyData *veryfydata;
	veryfydata = New(VeryfyData);
	veryfydata->checkData = NewLBS();
	veryfydata->redirectData = NewLBS();
	return veryfydata;
}

static	void
FreeVeryfyData(
	VeryfyData	*veryfydata)
{
	FreeLBS(veryfydata->checkData);
	FreeLBS(veryfydata->redirectData);
	xfree(veryfydata);
	veryfydata = NULL;
}

static Ticket *
NewTicket(void)
{
	Ticket *ticket;
ENTER_FUNC;
	ticket = New(Ticket);
	ticket->ticket_id = 0;
	ticket->fd = 0;
	ticket->veryfydata = NULL;
	ticket->status = TICKET_BEGIN;
LEAVE_FUNC;
	return ticket;
}

static Ticket *
LookupTicket(
	uint64_t	ticket_id,
	int fd)
{
	GSList *list;
	Ticket *ticket;
	
	list = TicketList;
	while(list){
		if (list->data) {
			ticket = (Ticket *)list->data;
			if ( (ticket->ticket_id == ticket_id)
				 && (ticket->fd == fd) ) {
				return ticket;
			}
		}
		list = list->next;
	}
	return NULL;
}


static void
OrderTicket(
	NETFILE	*fpLog)
{
	Ticket *ticket;
ENTER_FUNC;
	pthread_mutex_lock(&redlock);
	ticket = NewTicket();
	ticket->fd = fpLog->fd;
	ticket->ticket_id = TICKETID++;
	TicketList = g_slist_append(TicketList, ticket);
	SendUInt64(fpLog, ticket->ticket_id);
	pthread_mutex_unlock(&redlock);
LEAVE_FUNC;
}

static void
RecvRedData(
	NETFILE	*fpLog)
{
	uint64_t	ticket_id;
	Ticket		*ticket;
	VeryfyData	*veryfydata;
ENTER_FUNC;
	veryfydata = NewVerfyData();
	ticket_id = RecvUInt64(fpLog);
	RecvLBS(fpLog, veryfydata->checkData);
	LBS_EmitEnd(veryfydata->checkData);
	RecvLBS(fpLog, veryfydata->redirectData);
	LBS_EmitEnd(veryfydata->redirectData);	
	ticket = LookupTicket(ticket_id, fpLog->fd);
	if ( (ticket != NULL)
		 && (ticket->status == TICKET_BEGIN) ) {
		ticket->veryfydata = veryfydata;
		ticket->status = TICKET_DATA;
	} else {
		FreeVeryfyData(veryfydata);
	}
LEAVE_FUNC;
}

static Ticket *
DequeueTicket(void)
{
	GSList *first = NULL;
	Ticket *ticket = NULL;

	while ( ticket == NULL ) {
		pthread_mutex_lock(&redlock);
		first = g_slist_nth(TicketList,0);
		if ( (fShutdown) && (first == NULL) ){
			break;
		}
		if ( first ) {
			ticket = (Ticket *)(first->data);
			if ( (ticket->status == TICKET_COMMIT)
				 || (ticket->status == TICKET_ABORT) ) {
				TicketList = g_slist_remove_link(TicketList,first);
				g_slist_free_1(first);
			} else {
				ticket = NULL;
			}
		}
		if ( ticket == NULL ) {
			pthread_cond_wait(&redcond,&redlock);
		}
		pthread_mutex_unlock(&redlock);
	}
	return ticket;
}

static void
CommitTicket(
	NETFILE	*fpLog)
{
	uint64_t	ticket_id;
	Ticket		*ticket;

	ticket_id = RecvUInt64(fpLog);
	ticket = LookupTicket(ticket_id, fpLog->fd);
	if (ticket) {
		if (ticket->status != TICKET_ABORT) {
			ticket->status = TICKET_COMMIT;
		}
		SendPacketClass(fpLog,RED_OK);	
	} else {
		SendPacketClass(fpLog,RED_NOT);			
	}
	pthread_cond_signal(&redcond);
}

static void
AbortTicket(
	NETFILE	*fpLog)
{
	Ticket *ticket;
	uint64_t	ticket_id;
ENTER_FUNC;
	ticket_id = RecvUInt64(fpLog);
	ticket = LookupTicket(ticket_id, fpLog->fd);
	if (ticket) {
		ticket->status = TICKET_ABORT;
	}
	pthread_cond_signal(&redcond);
LEAVE_FUNC;
}

static void
AllAbortTicket(
	NETFILE	*fpLog)
{
	GSList *list;
	Ticket *ticket;
ENTER_FUNC;
	for (list = TicketList; list; list=list->next){
		ticket = (Ticket *)list->data;
		if ( (ticket != NULL )
			 && (ticket->fd == fpLog->fd)
			 && ((ticket->status == TICKET_BEGIN)
				 || (ticket->status == TICKET_DATA)) ) {
			ticket->status = TICKET_ABORT;
			Warning("Auto abort %llu\n", ticket->ticket_id);
		}
	}
	pthread_cond_signal(&redcond);
LEAVE_FUNC;
}

static	void
LogThread(
	void	*para)
{
	int		fhLog = (int)(long)para;
	NETFILE	*fpLog;
	PacketClass	c;
	Bool	fSuc = TRUE;
ENTER_FUNC;
	dbgmsg("log thread!\n");
	fpLog = SocketToNet(fhLog);
	do {
		switch	( c = RecvPacketClass(fpLog) ) {
		  case	RED_BEGIN:
			OrderTicket(fpLog);
			break;
		  case	RED_DATA:
			RecvRedData(fpLog);
			break;
		  case	RED_COMMIT:
			CommitTicket(fpLog);
			break;
		  case	RED_ABORT:
			AbortTicket(fpLog);
			break;
		  case	RED_PING:
			SendPacketClass(fpLog,RED_PONG);
			break;
		  case	RED_STATUS:
			SendChar(fpLog, ThisDBG->process[PROCESS_UPDATE].dbstatus);
			break;
		  case	RED_END:
			fSuc = FALSE;
			break;
		  default:
			SendPacketClass(fpLog,RED_NOT);
			fSuc = FALSE;
			break;
		}
	}	while	(  !fShutdown && fSuc && fpLog->fOK );
	AllAbortTicket(fpLog);
	CloseNet(fpLog);
	dbgmsg("log thread close!\n");
LEAVE_FUNC;
}

extern	pthread_t
ConnectLog(
	int		_fhLog)
{
	int		fhLog;
	pthread_t	thr;

ENTER_FUNC;
	if		(  ( fhLog = accept(_fhLog,0,0) )  <  0  )	{
		printf("_fhLog = %d\n",_fhLog);
		Error("accept: ", strerror(errno));					
	}
	pthread_create(&thr,NULL,(void *(*)(void *))LogThread,(void *)(long)fhLog);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

static  FILE	*
OpenLogFile(
	char	*file)
{
	FILE	*fp = NULL;

	if		(  ThisDBG->file  !=  NULL  ) {
		umask((mode_t) 0077);
		if		(  ( fp = fopen(ThisDBG->file,"a+") )  ==  NULL  ) {
			Warning("can not open log file :%s", ThisDBG->file);
		}
	}
	return fp;
}

static  void
WriteLog(
	FILE	*fp,
	char	*state)
{
	time_t	nowtime;
	struct	tm	*Now;
ENTER_FUNC;
	if		(  fp  !=  NULL  ) {
		time(&nowtime);
		Now = localtime(&nowtime);
		fprintf(fp, "%s %04d/%02d/%02d/%02d:%02d:%02d/ ========== %s ========== %s\n"
				,ThisDBG->func->commentStart
				, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
				, Now->tm_hour,Now->tm_min,Now->tm_sec,state
				,ThisDBG->func->commentEnd);
		fflush(fp);
	}
LEAVE_FUNC;
}

static  void
WriteLogQuery(
	FILE	*fp,
	char	*query)
{
	static  int count = 0;
	time_t	nowtime;
	struct	tm	*Now;

ENTER_FUNC;
	if		(  fp  !=  NULL  ) {
		time(&nowtime);
		Now = localtime(&nowtime);
		fprintf(fp,"%s %04d/%02d/%02d/%02d:%02d:%02d/%08d %s"
				,ThisDBG->func->commentStart
				, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
				, Now->tm_hour,Now->tm_min,Now->tm_sec,count
				,ThisDBG->func->commentEnd);
		fprintf(fp,"%s\n", query);
		fflush(fp);
		count ++;
	}
LEAVE_FUNC;
}

static  Bool
ConnectDB(void)
{
	Bool rc = TRUE;
ENTER_FUNC;
	OpenRedirectDB(ThisDBG);
	if (  ThisDBG->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT ) {
		Message("connect to database successed");
		ThisDBG->checkData = NewLBS();
	} else {
		Message("connect to database failed");
		rc = FALSE;
	}
LEAVE_FUNC;
	return rc;
}

void
ReConnectDB(void)
{
	int	retry = 0;
ENTER_FUNC;
	while ( !ConnectDB() ){
		retry ++;
		if ( retry > MaxSendRetry ){
			break;
		}
		sleep (CONNECT_INTERVAL);
	}
	if ( ThisDBG->process[PROCESS_UPDATE].dbstatus == DB_STATUS_UNCONNECT ){
		ThisDBG->process[PROCESS_UPDATE].dbstatus = DB_STATUS_FAILURE;
	}
LEAVE_FUNC;
}

static int
CheckRedirectData(
	LargeByteString	*src,
	LargeByteString	*dsc)
{
	int rc = MCP_OK;
ENTER_FUNC;
	if ( strcmp(LBS_Body(src), LBS_Body(dsc)) == 0){
		rc = MCP_OK;
	} else {
		Warning("CheckData difference %s %s", LBS_Body(src), LBS_Body(dsc));
		rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	rc;
}

extern	int
WriteDB(
	LargeByteString	*query,
	LargeByteString	*orgcheck)
{
	int rc;
	char buff[SIZE_BUFF];
	LargeByteString	*redcheck;
ENTER_FUNC;
	rc = TransactionRedirectStart(ThisDBG);
	if ( rc == MCP_OK ) {
		rc = ExecRedirectDBOP(ThisDBG, LBS_Body(query), DB_UPDATE);
		redcheck = ThisDBG->checkData;
	}
	if ( rc == MCP_OK ) {
		if ( ( !fNoSumCheck) &&  ( LBS_Size(orgcheck) > 0 ) ){
			rc = CheckRedirectData(orgcheck, redcheck);
			if ( rc != MCP_OK ) {
				snprintf(buff, 60, "Difference for the update check %s...",(char *)LBS_Body(query));
				Warning(buff);
			}
		}
	}
	if ( rc == MCP_OK ) {
		rc = TransactionRedirectEnd(ThisDBG);
	}
LEAVE_FUNC;
	return rc;
}


static	int
ExecDB(
	VeryfyData *veryfydata)
{
	int rc = MCP_OK;
ENTER_FUNC;
	if ( ThisDBG->process[PROCESS_UPDATE].dbstatus == DB_STATUS_UNCONNECT ) {
		ReConnectDB();
	}
	if ( ThisDBG->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT ){	
		rc = WriteDB(veryfydata->redirectData, veryfydata->checkData);
	}
	if ( rc != MCP_OK ) {
		CloseRedirectDB(ThisDBG);
		if ( rc == MCP_BAD_CONN ) {
			ThisDBG->process[PROCESS_UPDATE].dbstatus = DB_STATUS_UNCONNECT;
		} else {
			ThisDBG->process[PROCESS_UPDATE].dbstatus = DB_STATUS_FAILURE;
		}
	}
LEAVE_FUNC;
	return ThisDBG->process[PROCESS_UPDATE].dbstatus;
}

static  void
ReRedirect(
	char	*query)
{
ENTER_FUNC;
	BeginDB_Redirect(ThisDBG);
	PutDB_Redirect(ThisDBG, query);
	CommitDB_Redirect(ThisDBG);
LEAVE_FUNC;
}

static void
CheckFailure(
	FILE	*fp)
{
	if ( ThisDBG->process[PROCESS_UPDATE].dbstatus == DB_STATUS_FAILURE ){
		WriteLog(fp, "DB synchronous failure");		
		Warning("DB synchronous failure");
		ThisDBG->process[PROCESS_UPDATE].dbstatus = DB_STATUS_DISCONNECT;
	}
}


static	void
FileThread(
	void	*dummy)
{
	VeryfyData *veryfydata;
	char	*dbname;
	FILE	*fp;
	Ticket *ticket;

ENTER_FUNC;
	fp = OpenLogFile(ThisDBG->file);
	if		(  ( dbname = GetDB_DBname(ThisDBG,DB_UPDATE) )  !=  NULL  ) {
		if (!fNoSumCheck) {
			WriteLog(fp, "dbredirector start");
		} else {
			WriteLog(fp, "dbredirector start(No sum check)");
		}
		ConnectDB();
	} else {
		WriteLog(fp, "dbredirector start(No database)");
		OpenDB_RedirectPort(ThisDBG);
		ThisDBG->process[PROCESS_UPDATE].dbstatus = DB_STATUS_NOCONNECT;
	}
	while	( TRUE )	{
		ticket = DequeueTicket();
		if ( ticket == NULL )
			break;
		if ( ticket->status == TICKET_COMMIT ) {
			veryfydata = ticket->veryfydata;
			if (veryfydata != NULL) {
				if (LBS_Size(veryfydata->redirectData) > 0) {
					if ( ExecDB(veryfydata) == DB_STATUS_UNCONNECT ){
						/* retry */						
						ExecDB(veryfydata);
					}
					CheckFailure(fp);
					ReRedirect(LBS_Body(veryfydata->redirectData));
					WriteLogQuery(fp, LBS_Body(veryfydata->redirectData));
				}
				FreeVeryfyData(veryfydata);
			}
		}
		xfree(ticket);
	}
	if		(  dbname  ==  NULL  ) {
		CloseDB_RedirectPort(ThisDBG);
	}
	CleanUNIX_Socket(RedirectPort);
	WriteLog(fp, "dbredirector stop");
LEAVE_FUNC;
}


extern	void
ExecuteServer(void)
{
	int		_fhLog;
	fd_set	ready;
	int		maxfd;

ENTER_FUNC;
	pthread_mutex_init(&redlock,NULL);
	pthread_cond_init(&redcond, NULL);
	pthread_create(&_FileThread,NULL,(void *(*)(void *))FileThread,NULL); 
	_fhLog = InitServerPort(RedirectPort,Back);
	maxfd = _fhLog;

	while	(!fShutdown)	{
		FD_ZERO(&ready);
		FD_SET(_fhLog,&ready);
		if (select(maxfd+1,&ready,NULL,NULL,NULL) < 0) {
			if (errno == EINTR) {
				continue;
			}
			Error("select: ", strerror(errno));			
		}
		if		(  FD_ISSET(_fhLog,&ready)  ) {
			ConnectLog(_fhLog);
		}
	}
	pthread_cond_signal(&redcond);
	pthread_join(_FileThread,NULL);
LEAVE_FUNC;
}

#ifdef	DEBUG
static	void
DumpDBG(
	char		*name,
	DBG_Struct	*dbg,
	void		*dummy)
{
	printf("name     = [%s]\n",dbg->name);
	printf("\ttype     = [%s]\n",dbg->type);
	printf("\tDB name  = [%s]\n",GetDB_DBname(dbg,DB_UPDATE));
	printf("\tDB user  = [%s]\n",GetDB_User(dbg,DB_UPDATE));
	printf("\tDB pass  = [%s]\n",GetDB_Pass(dbg,DB_UPDATE));
 	printf("\tDB sslmode  = [%s]\n",GetDB_Sslmode(dbg,DB_UPDATE));

	if		(  dbg->file  !=  NULL  ) {
		printf("\tlog file = [%s]\n",dbg->file);
	}
	if		(  dbg->redirect  !=  NULL  ) {
		dbg = dbg->redirect;
		printf("\tredirect = [%s]\n",dbg->name);
	}
}
#endif

static	void
_CheckDBG(
	char		*name,
	DBG_Struct	*dbg,
	char		*red_name)
{
	DBG_Struct	*red_dbg;
	char *src_port, *dsc_port;
	char *dbg_dbname = "", *red_dbg_dbname = "";
	char	*dbname;
ENTER_FUNC;		
	if		(  dbg->redirect  !=  NULL  ) {
		red_dbg = dbg->redirect;
		if ( strcmp(red_dbg->name, red_name ) == 0 ){
			src_port = StrDup(StringPort(GetDB_Port(dbg,DB_UPDATE)));
			dsc_port = StrDup(StringPort(GetDB_Port(red_dbg,DB_UPDATE)));
			if		(  ( dbname = GetDB_DBname(dbg,DB_UPDATE) )  !=  NULL  ) {
				dbg_dbname = dbname;
			}
			if		(  ( dbname = GetDB_DBname(red_dbg,DB_UPDATE) )  !=  NULL  ) {
				red_dbg_dbname = dbname;
			}
			if ( ( strcmp(dbg->type, red_dbg->type ) == 0 )
				 && ( strcmp(dbg_dbname, red_dbg_dbname ) == 0 )
				 && ( strcmp(src_port, dsc_port) == 0 ) ) {
				Error("The connection destination is same DB");
			}
			xfree(src_port);
			xfree(dsc_port);
		}
	}
LEAVE_FUNC;
}

static	void
CheckDBG(
	char		*name)
{
#ifdef	DEBUG
	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)DumpDBG,NULL);
#endif
	if		(  ( ThisDBG = (DBG_Struct *)g_hash_table_lookup(ThisEnv->DBG_Table,name) )
			   ==  NULL  ) {
		Error("DB group not found");
	}
	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)_CheckDBG,name);
}

void StopSystem( int no ) {
    ( void )no;
	fShutdown = TRUE;
	Message("receive stop signal");
}

extern	void
InitSystem(
	char	*name)
{
    struct sigaction sa;
ENTER_FUNC;
	InitNET();

	memset( &sa, 0, sizeof(struct sigaction) );
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	sigemptyset (&sa.sa_mask);	
	sigaction( SIGPIPE, &sa, NULL );

	sa.sa_handler = StopSystem;
	sa.sa_flags |= SA_RESTART;
	sigemptyset (&sa.sa_mask);
	sigaction( SIGUSR1, &sa, NULL );

	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,FALSE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	InitDB_Process(NULL);

	CheckDBG(name);

	if		(  PortNumber  ==  NULL  ) {
		if		(  ( ThisDBG != NULL) 
				   && (ThisDBG->redirectPort  !=  NULL )) {
			RedirectPort = ThisDBG->redirectPort;
		} else {
			RedirectPort = ParPortName(PORT_REDIRECT);
		}
	} else {
		RedirectPort = ParPortName(PortNumber);
	}
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"waiting port name"	 							},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"connection waiting queue number"				},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"environment file name"							},
	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"base directory"			 					},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"record directory"			 					},
	{	"ddir",		STRING,		TRUE,	(void*)&D_Dir,
		"*D directory"				 					},

	{	"host",		STRING,		TRUE,	(void*)&DB_Host,
		"DB host name"									},
	{	"port",		STRING,		TRUE,	(void*)&DB_Port,
		"DB port number"								},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"DB name"										},
	{	"user",		STRING,		TRUE,	(void*)&DB_User,
		"DB user name"									},
	{	"pass",		STRING,		TRUE,	(void*)&DB_Pass,
		"DB password"									},
	{	"sslmode",	STRING,		TRUE,	(void*)&DB_Sslmode,
		"DB SSL mode"									},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"no check dbredirector start"					},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"don't use dbredirector"						},
	{	"nosumcheck",BOOLEAN,	TRUE,	(void*)&fNoSumCheck,
		"no count dbredirector updates"					},
	{	"maxretry",	INTEGER,	TRUE,	(void*)&MaxSendRetry,
		"send retry dbredirector"						},
	{	"retryint",	INTEGER,	TRUE,	(void*)&RetryInterval,
		"retry interval of dbredirector(sec)"			},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Sslmode = NULL;
	DB_Name = DB_User;

	fNoCheck = FALSE;
	fNoSumCheck = FALSE;
	fNoRedirect = FALSE;
	MaxSendRetry = 3;
	RetryInterval = 5;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*name;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("dbredirector",NULL);

	if		(	fl	&&	fl->name  ) {
		name = fl->name;
	} else {
		name = "";
	}
	InitSystem(name);
	Message("dbredirector start");
	ExecuteServer();
	Message("dbredirector end");
	return	(0);
}
