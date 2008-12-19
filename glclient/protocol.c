/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*
#define	NETWORK_ORDER
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#include	<iconv.h>

#include	"types.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"comm.h"
#include	"protocol.h"
#define		_PROTOCOL_C
#include	"message.h"
#include	"debug.h"
#include	"marshaller.h"
#include	"interface.h"
#include	"gettext.h"
#include	"const.h"

#ifdef	NETWORK_ORDER
#define	RECV32(v)	ntohl(v)
#define	RECV16(v)	ntohs(v)
#define	SEND32(v)	htonl(v)
#define	SEND16(v)	htons(v)
#else
#define	RECV32(v)	(v)
#define	RECV16(v)	(v)
#define	SEND32(v)	(v)
#define	SEND16(v)	(v)
#endif

static	LargeByteString	*LargeBuff;

#if	0
static	GList		*ScreenStack;
static	void
InitScreenStack(void)
{
	ScreenStack = NULL;
}

static	void
PushScreenStack(
	char	*name)
{
	ScreenStack = g_list_append(ScreenStack,name);
}

static	char	*
BottomScreenStack(void)
{
	GList	*last;
	char	*ret;

	if		(  ScreenStack  !=  NULL  ) {
		last = g_list_last(ScreenStack);
		ret = (char *)last->data;
	} else {
		ret = NULL;
	}
	return	(ret);
}

static	void
PopScreenStack(void)
{
	GList	*last;

	last = g_list_last(ScreenStack);
	ScreenStack = g_list_remove_link(ScreenStack,last);
}
#endif

/*
 *	misc functions
 */

static	void
ConvEUCJP2UTF8(
	char	*instr,
	size_t	*insize,
	char	*outstr,
	size_t	*outsize)
{
	iconv_t	cd;
	char	*ostr;

	cd = iconv_open("utf8", "EUC-JP");
	dbgprintf("size = %d\n",(int)strlen(str));
	ostr = outstr;
	if (*insize > 0) {
		if (iconv(cd,&instr,insize,&ostr,outsize) != 0) {
			dbgprintf("error = %d\n",errno);
		}
		iconv_close(cd);
	}
	*ostr = 0;
}

static	void
ConvUTF82EUCJP(
	char	*instr,
	size_t	*insize,
	char	*outstr,
	size_t	*outsize)
{
	iconv_t	cd;
	char	*ostr;

	cd = iconv_open("EUC-JP", "utf8");
	dbgprintf("size = %d\n",(int)strlen(str));
	ostr = outstr;
	if (*insize > 0) {
		if (iconv(cd,&instr,insize,&ostr,outsize) != 0) {
			dbgprintf("error = %d\n",errno);
		}
		iconv_close(cd);
	}
	*ostr = 0;
}

/*
 *	send/recv functions
 */

#define	SendChar(fp,c)	nputc((c),(fp))
#define	RecvChar(fp)	ngetc(fp)

static void
GL_Error(void)
{
	UI_ErrorDialog(_("Connection lost\n"));
}

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
}

extern	PacketClass
GL_RecvPacketClass(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

extern	PacketDataType
GL_RecvDataType(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data)
{
	byte	buff[sizeof(int)];

	*(int *)buff = SEND32(data);
	Send(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

extern	int
GL_RecvInt(
	NETFILE	*fp)
{
	byte	buff[sizeof(int)];

	Recv(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	(RECV32(*(int *)buff));
}

static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data)
{
	byte	buff[sizeof(int)];

	*(int *)buff = SEND32(data);
	Send(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	size_t
GL_RecvLength(
	NETFILE	*fp)
{
	byte	buff[sizeof(int)];

	Recv(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	((size_t)RECV32(*(int *)buff));
}
#if	0
static	unsigned	int
GL_RecvUInt(
	NETFILE	*fp)
{
	byte	buff[sizeof(int)];

	Recv(fp,buff,sizeof(unsigned int));
	return	(RECV32(*(unsigned int *)buff));
}

static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int		data)
{
	byte	buff[sizeof(int)];

	*(unsigned int *)buff = SEND32(data);
	Send(fp,buff,sizeof(unsigned int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

#endif

static	void
GL_RecvString(
	NETFILE	*fp,
	size_t  maxsize,
	char	*str)
{
	size_t		lsize;
	static char	buff[SIZE_LARGE_BUFF * 2];
	char		*optr;

ENTER_FUNC;
	lsize = GL_RecvLength(fp);
	if		(	maxsize > lsize 	){
		if	(UIVERSION(glSession) == UI_VERSION_1) {
			Recv(fp,str,lsize);
			if		(  !CheckNetFile(fp)  ) {
				GL_Error();
			}
			str[lsize] = 0;
		}	else	{
			Recv(fp,buff, lsize);
			if		(  !CheckNetFile(fp)  ) {
				GL_Error();
			}
			buff[lsize] = 0;
			optr = str;
			ConvEUCJP2UTF8(buff,&lsize, optr, &maxsize);
		}
	} else {
		UI_ErrorDialog(_("error size mismatch !"));
	}
LEAVE_FUNC;
}

extern	void
GL_RecvName(
	NETFILE	*fp,
	size_t  size,
	char	*name)
{
ENTER_FUNC;
	GL_RecvString( fp, size, name);
LEAVE_FUNC;
}

static	void
GL_SendString(
	NETFILE	*fp,
	char	*str)
{
	size_t		size;
	size_t		osize;
	static char	buff[SIZE_LARGE_BUFF * 2];
	char		*optr;

ENTER_FUNC;
	if (str != NULL) { 
		size = strlen(str);
		optr = str;
		if (UIVERSION(glSession) == UI_VERSION_2) {
			optr = buff;
			osize = SIZE_LARGE_BUFF * 2;
			ConvUTF82EUCJP(str, &size, optr, &osize);
			size = SIZE_LARGE_BUFF * 2 - osize;
		}
	} else {
		optr = NULL;
		size = 0;
	}
	GL_SendLength(fp,size);
	if (!CheckNetFile(fp)) {
		GL_Error();
	}
	if (size > 0) {
		Send(fp,optr,size);
		if (!CheckNetFile(fp)) {
			GL_Error();
		}
	}
LEAVE_FUNC;
}

extern	void
GL_SendName(
	NETFILE	*fp,
	char	*name)
{
	size_t	size;

ENTER_FUNC;
	if		(   name  !=  NULL  ) { 
		size = strlen(name);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size);
	if		(  size  >  0  ) {
		Send(fp,name,size);
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
	}
LEAVE_FUNC;
}

extern	void
GL_SendDataType(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
}
#if	0
static	void
GL_SendObject(
	NETFILE	*fp,
	MonObjectType	obj)
{
	GL_SendUInt(fp,(unsigned int)obj);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	MonObjectType
GL_RecvObject(
	NETFILE	*fp)
{
	return	((MonObjectType)GL_RecvUInt(fp));
}
#endif
static	void
GL_SendFixed(
	NETFILE	*fp,
	Fixed	*xval)
{
	GL_SendLength(fp,xval->flen);
	GL_SendLength(fp,xval->slen);
	GL_SendString(fp,xval->sval);
}

static	Fixed	*
GL_RecvFixed(
	NETFILE	*fp)
{
	Fixed	*xval;

ENTER_FUNC;
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp);
	xval->slen = GL_RecvLength(fp);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp, (xval->flen + 1), xval->sval);
LEAVE_FUNC;
	return	(xval); 
}

static	double
GL_RecvFloat(
	NETFILE	*fp)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	(data);
}

static	void
GL_SendFloat(
	NETFILE	*fp,
	double	data)
{
	Send(fp,&data,sizeof(data));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	Bool
GL_RecvBool(
	NETFILE	*fp)
{
	char	buf[1];

	Recv(fp,buf,1);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

static	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	void
GL_RecvLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	size_t	size;
ENTER_FUNC;
	size = GL_RecvLength(fp);
	LBS_ReserveSize(lbs,size,FALSE);
	if		(  size  >  0  ) {
		Recv(fp,LBS_Body(lbs),size);
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
	} else {
		dbgmsg("Recv LBS 0 size.");
	}
LEAVE_FUNC;
}

static	void
GL_SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	GL_SendLength(fp,LBS_Size(lbs));
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
	}
}

//////////////////////////////////////////////////////////////////////

static	Bool
RecvFile(
	NETFILE	*fpC,
	char	*name,
	char	*fname)
{
	FILE		*fp;
	size_t		size
	,			left
	,			totalsize
	,			currentsize;
	char		buff[SIZE_BUFF];
	char		buffeuc[SIZE_LARGE_BUFF];
	char		buffutf8[SIZE_LARGE_BUFF * 2];
	char		ffname[SIZE_BUFF];
	Bool		ret;
	gchar 		*tmpfile, *dirname;
	int 		fd;

ENTER_FUNC;
	GL_SendPacketClass(fpC,GL_GetScreen);
	GL_SendString(fpC,name);
	if		(  fMlog  ) {
		sprintf(buff,"recv screen file [%s]\n",name);
		MessageLog(buff);
	}
	if		(  GL_RecvPacketClass(fpC)  ==  GL_ScreenDefine  ) {
		// download
		left = totalsize = (size_t)GL_RecvInt(fpC);
		currentsize = 0;
		do {
			if		(left  >  SIZE_BUFF  ) {
				size = SIZE_BUFF;
			} else {
				size = left;
			}
			size = Recv(fpC,buff,size);
			if		(  size  >  0  ) {
				memcpy(buffeuc + currentsize, buff, size);
				left -= size;
				currentsize += size;
			}
		}	while	(  left  >  0  );

		// write euc file
		tmpfile = g_strconcat(fname, "gl_cache_XXXXXX", NULL);
		dirname = g_dirname(tmpfile);
		MakeCacheDir(dirname);
		g_free(dirname);
		if  ((fd = mkstemp(tmpfile)) == -1 ) {
			UI_ErrorDialog(_("could not write tmp file"));
		}
		if	((fp = fdopen(fd,"w")) == NULL) {
			UI_ErrorDialog(_("could not write cache file"));
		}
		fwrite(buffeuc, 1, totalsize, fp);
		fchmod(fileno(fp), 0600);
		fclose(fp);
		rename(tmpfile, fname);
		unlink(tmpfile);
		g_free(tmpfile);

		// convert to utf8
		if (UIVERSION(glSession) == UI_VERSION_2) {
			left = SIZE_LARGE_BUFF * 2;
			ConvEUCJP2UTF8(buffeuc, &totalsize, buffutf8, &left);
			totalsize = SIZE_LARGE_BUFF * 2 - left;
			// write utf8 file
			snprintf(ffname,SIZE_BUFF , "%s.utf8", fname);
			tmpfile = g_strconcat(ffname, "gl_cache_XXXXXX", NULL);
			dirname = g_dirname(tmpfile);
			MakeCacheDir(dirname);
			g_free(dirname);
			if  ((fd = mkstemp(tmpfile)) == -1 ) {
				UI_ErrorDialog(_("could not write tmp file"));
			}
			if	((fp = fdopen(fd,"w")) == NULL) {
				UI_ErrorDialog(_("could not write cache file"));
			}
			fwrite(buffutf8, 1, totalsize, fp);
			fchmod(fileno(fp), 0600);
			fclose(fp);
			rename(tmpfile, ffname);
			unlink(tmpfile);
			g_free(tmpfile);
		}
		ret = TRUE;
	} else {
		UI_ErrorDialog(_("invalid protocol sequence"));
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
CheckScreens(
	NETFILE		*fp,
	Bool		fInit)
{	
	char		sname[SIZE_BUFF];
	char		sname2[SIZE_BUFF];
	char		*fname;
	struct	stat	stbuf;
	time_t		stmtime
	,			stctime;
	off_t		stsize;
	PacketClass	klass;

ENTER_FUNC;
	while		(  ( klass = GL_RecvPacketClass(fp) )  ==  GL_QueryScreen  ) {
dbgmsg("*");
		GL_RecvString(fp, sizeof(sname), sname);
		stsize = (off_t)GL_RecvInt(fp);
		stmtime = (time_t)GL_RecvInt(fp);
		stctime = (time_t)GL_RecvInt(fp);
		fname = CacheFileName(sname);

		if		(	(  stat(fname,&stbuf)  !=  0        )
				 ||	(  stbuf.st_mtime      <   stmtime  )
				 ||	(  stbuf.st_ctime      <   stctime  )
				 ||	(  stbuf.st_size       !=  stsize   ) ) {
			RecvFile(fp, sname, fname);
		} else {
			if (UIVERSION(glSession) == UI_VERSION_2) {
				snprintf(sname2, SIZE_BUFF, "%s.utf8", fname);
				fname = (char *)sname2;
				if (stat(fname,&stbuf) != 0) {
					fname = (char *)sname;
					RecvFile(fp, sname, fname);
				} else {
					GL_SendPacketClass(fp, GL_NOT);
				}
			} else {
				GL_SendPacketClass(fp, GL_NOT);
			}
		}
		if		(  fInit  ) {
			UI_ShowWindow(sname,SCREEN_NEW_WINDOW);
			fInit = FALSE;
		}
	}
LEAVE_FUNC;
}

static	void
RecvValueSkip(
	NETFILE			*fp,
	PacketDataType	type)
{
	char			name[SIZE_BUFF];
	char			buff[SIZE_BUFF];
	int				count
	,				i;

ENTER_FUNC;
	if		(  type  ==  GL_TYPE_NULL  ) {
		type = GL_RecvDataType(fp);
	}
	switch	(type) {
	  case	GL_TYPE_INT:
		(void)GL_RecvInt(fp);
		break;
	  case	GL_TYPE_BOOL:
		(void)GL_RecvBool(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_OBJECT:
		GL_RecvLBS(fp, LargeBuff);
		break;
	  case	GL_TYPE_NUMBER:
		FreeFixed(GL_RecvFixed(fp));
		break;
	  case	GL_TYPE_ARRAY:
		count = GL_RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			RecvValueSkip(fp,GL_TYPE_NULL);
		}
		break;
	  case	GL_TYPE_RECORD:
		count = GL_RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			GL_RecvString(fp, sizeof(name), name);
			RecvValueSkip(fp,GL_TYPE_NULL);
		}
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

extern	void
RecvValue(
	NETFILE		*fp,
	char		*widgetName)
{
	Bool			fTrace;
	PacketDataType	type;
	int				count
	,				i;
	char			name[SIZE_BUFF]
	,				childWidgetName[SIZE_BUFF]
	,				*dataname;
	Bool			fDone;

ENTER_FUNC;
	dbgprintf("WidgetName = [%s]\n",widgetName);
	if (UI_IsWidgetName(ThisWindowName)) {
		fDone = FALSE;
		fTrace = TRUE;
		if (Protocol1) {
			if (UI_IsWidgetName(widgetName)) {
				if (RecvWidgetData(widgetName,fp)) {
					fTrace = FALSE;
				}
				fDone = TRUE;
			} else {
				if (!Protocol2) {
					fTrace = FALSE;	/*	fatal error	*/
					fDone = TRUE;
					RecvValueSkip(fp,GL_TYPE_NULL);
				}
			}
		}
		if (Protocol2) {
			if (!fDone) {
				if ((dataname = strchr(widgetName,'.'))  !=  NULL) {
					dataname ++;
				}
				dbgprintf("dataname = [%s]\n",dataname);
				if (UI_IsWidgetName2(dataname)) {
					if (RecvWidgetData(widgetName,fp)) {
						fTrace = FALSE;
					}
					fDone = TRUE;
				}
			}
		}
		if (!fDone) {
			fTrace = TRUE;
		}
	} else {
		fTrace = FALSE;	/*	fatal error	*/
		RecvValueSkip(fp,GL_TYPE_NULL);
	}
	if (fTrace) {
		type = GL_RecvDataType(fp);
		switch	(type) {
		  case	GL_TYPE_RECORD:
			count = GL_RecvInt(fp);
			for	(  i = 0 ; i < count ; i ++ ) {
				GL_RecvString(fp, sizeof(name), name);
				sprintf(childWidgetName,"%s.%s",widgetName,name);
				RecvValue(fp,childWidgetName);
			}
			break;
		  case	GL_TYPE_ARRAY:
			count = GL_RecvInt(fp);
			for	(  i = 0 ; i < count ; i ++ ) {
				sprintf(childWidgetName, "%s[%d]",widgetName, i);
				RecvValue(fp,childWidgetName);
			}
			break;
		  default:
			RecvValueSkip(fp,type);
			break;
		}
	}
LEAVE_FUNC;
}

extern	Bool
GetScreenData(
	NETFILE		*fp)
{
	char		window[SIZE_BUFF]
	,			widgetName[SIZE_BUFF];
	PacketClass	c;
	byte		type;
	char		buff[SIZE_BUFF];

ENTER_FUNC;
	fInRecv = TRUE; 
	if (ThisWindowName != NULL)
		g_free(ThisWindowName);
	CheckScreens(fp,FALSE);	 
	GL_SendPacketClass(fp,GL_GetData);
	GL_SendInt(fp,0);				/*	get all data	*/
	while	(  ( c = GL_RecvPacketClass(fp) )  ==  GL_WindowName  ) {
		GL_RecvString(fp, sizeof(window), window);
		if		(  fMlog  ) {
			sprintf(buff,"recv window [%s]\n",window);
			MessageLog(buff);
		}
		dbgprintf("[%s]\n",window);
		type = (byte)GL_RecvInt(FPCOMM(glSession)); 
		UI_ShowWindow(window,type);
		switch	(type) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			if ((c = GL_RecvPacketClass(fp)) == GL_ScreenData) {
				ThisWindowName = strdup(window);
				RecvValue(fp,window);
				UI_UpdateScreen(window);
			}
			if (type == SCREEN_CHANGE_WINDOW) {
				UI_ResetScrolledWindow(window);
			}
			break;
		  default:
			c = GL_RecvPacketClass(fp);
			break;
		}
		if		(  c  ==  GL_NOT  ) {
			/*	no screen data	*/
		} else {
			/*	fatal error	*/
		}
	}
	if (c == GL_FocusName) {
		GL_RecvString(fp, sizeof(window), window);
		GL_RecvString(fp, sizeof(widgetName), widgetName);
		UI_GrabFocus(window, widgetName);
		c = GL_RecvPacketClass(fp);
	}
	UI_ResetTimer(window);
	fInRecv = FALSE;
LEAVE_FUNC;
	return TRUE;
}

static	void
GL_SendVersionString(
	NETFILE		*fp)
{
	char	*version;
	size_t	size;

#ifdef	NETWORK_ORDER
	if (UI_Version() == UI_VERSION_1) {
		version = "version:no:blob:expand:ps";
	} else {
#	ifdef	USE_PDF
		version = "version:no:blob:expand:pdf";
#	else
		version = "version:no:blob:expand:ps";
#	endif
	}
#else
	if (UI_Version() == UI_VERSION_1) {
		version = "version:blob:expand:ps";
	} else {
#	ifdef	USE_PDF
		version = "version:blob:expand:pdf";
#	else
		version = "version:blob:expand:ps";
#	endif
	}
#endif
	size = strlen(version);
	SendChar(fp,(size&0xFF));
	SendChar(fp,0);
	SendChar(fp,0);
	SendChar(fp,0);
	Send(fp,version,size);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

extern	Bool
SendConnect(
	NETFILE		*fp,
	char		*apl)
{
	Bool	rc;
	PacketClass	pc;

ENTER_FUNC;
	if		(  fMlog  ) {
		MessageLog(_("connection start\n"));
	}
	GL_SendPacketClass(fp,GL_Connect);
	GL_SendVersionString(fp);
	GL_SendString(fp,User);
	GL_SendString(fp,Pass);
	GL_SendString(fp,apl);
	if		(  ( pc = GL_RecvPacketClass(fp) )   ==  GL_OK  ) {
		rc = TRUE;
	} else {
		rc = FALSE;
		switch	(pc) {
		  case	GL_NOT:
			UI_ErrorDialog(_("can not connect server"));
			break;
		  case	GL_E_VERSION:
			UI_ErrorDialog(_("can not connect server(version not match)"));
			break;
		  case	GL_E_AUTH:
#ifdef USE_SSL
			if 	(fSsl) {
				UI_ErrorDialog(
					_("can not connect server(authentication error)"));
			} else {
				UI_ErrorDialog(
					_("can not connect server(user or password is incorrect)"));
			}
#else
			UI_ErrorDialog(
				_("can not connect server(user or password is incorrect)"));
#endif
			break;
		  case	GL_E_APPL:
			UI_ErrorDialog(
				_("can not connect server(application name invalid)"));
			break;
		  default:
			dbgprintf("[%X]\n",pc);
			UI_ErrorDialog(_("can not connect server(other protocol error)"));
			break;
		}
	}
LEAVE_FUNC;
	return	(rc);
}

extern	void
SendEvent(
	NETFILE		*fp,
	char		*window,
	char		*widget,
	char		*event)
{
	char		buff[SIZE_BUFF];

ENTER_FUNC;
	dbgprintf("window = [%s]",window); 
	dbgprintf("widget = [%s]",widget); 
	dbgprintf("event  = [%s]",event); 
	if		(  fMlog  ) {
		sprintf(buff,"send event  [%s:%s:%s]\n",window,widget,event);
		MessageLog(buff);
	}

	GL_SendPacketClass(fp,GL_Event);
	GL_SendString(fp,window);
	GL_SendString(fp,widget);
	GL_SendString(fp,event);
LEAVE_FUNC;
}

extern	void
InitProtocol(void)
{
ENTER_FUNC;
	LargeBuff = NewLBS();
#if	0
	InitScreenStack();
#endif
	ThisWindowName = NULL;
	WindowTable = NewNameHash();
	WidgetDataTable = NewNameHash();
LEAVE_FUNC;
}

static gboolean RemoveChangedWidget(gpointer	key,
	gpointer	value,
	gpointer	user_data) 
{
	g_free((char *)key);
	return TRUE;
}

static	void
_SendWindowData(
	char		*wname,
	WindowData	*wdata,
	gpointer	user_data)
{
ENTER_FUNC;
	GL_SendPacketClass(FPCOMM(glSession),GL_WindowName);
	GL_SendString(FPCOMM(glSession),wname);
	g_hash_table_foreach(wdata->ChangedWidgetTable,
		(GHFunc)SendWidgetData,FPCOMM(glSession));
	GL_SendPacketClass(FPCOMM(glSession),GL_END);

	if (wdata->ChangedWidgetTable != NULL) {
		g_hash_table_foreach_remove(wdata->ChangedWidgetTable, 
			RemoveChangedWidget, NULL);
		g_hash_table_destroy(wdata->ChangedWidgetTable);
	}
    wdata->ChangedWidgetTable = NewNameHash();
ENTER_FUNC;
}

extern	void
SendWindowData(void)
{
ENTER_FUNC;
	g_hash_table_foreach(WindowTable,(GHFunc)_SendWindowData,NULL);
	GL_SendPacketClass(FPCOMM(glSession),GL_END);
ENTER_FUNC;
}

extern	PacketDataType
RecvFixedData(
	NETFILE	*fp,
	Fixed	**xval)
{
	PacketDataType	type;
	
	type = GL_RecvDataType(fp);

	switch	(type) {
	  case	GL_TYPE_NUMBER:
		*xval = GL_RecvFixed(fp);
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		break;
	}
	return type;
}

extern	void
SendFixedData(
	NETFILE			*fp,
	PacketDataType	type,
	Fixed			*xval)
{
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,(char *)xval->sval);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFixed(fp,xval);
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		break;
	}
}

extern	void
SendStringData(
	NETFILE			*fp,
	PacketDataType	type,
	char			*str)
{
ENTER_FUNC;
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,(char *)str);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_OBJECT:
		LBS_ReserveSize(LargeBuff,strlen(str)+1,FALSE);
		strcpy(LBS_Body(LargeBuff),str);
		GL_SendLBS(fp,LargeBuff);
		break;
	  case	GL_TYPE_INT:
		GL_SendInt(fp,atoi(str));
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,atof(str));
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,(( *str == 'T' ) ? TRUE: FALSE ));
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

extern	PacketDataType
RecvStringData(
	NETFILE	*fp,
	char	*str,
	size_t	size)
{
	PacketDataType	type;

ENTER_FUNC;
	type = GL_RecvDataType(fp);
	switch	(type) {
	  case	GL_TYPE_INT:
		dbgmsg("int");
		sprintf(str,"%d",GL_RecvInt(fp));
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, size, str);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_OBJECT:
		dbgmsg("LBS");
		GL_RecvLBS(fp,LargeBuff);
		if		(  LBS_Size(LargeBuff)  >  0  ) {
			memcpy(str,LBS_Body(LargeBuff),LBS_Size(LargeBuff));
			str[LBS_Size(LargeBuff)] = 0;
		} else {
			*str = 0;
		}
		break;
	}
LEAVE_FUNC;
	return type;
}


extern	void
SendIntegerData(
	NETFILE			*fp,
	PacketDataType	type,
	int				val)
{
	char	buff[SIZE_BUFF];
	
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(buff,"%d",val);
		GL_SendString(fp,(char *)buff);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_INT:
		GL_SendInt(fp,val);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,(( val == 0 ) ? FALSE : TRUE ));
		break;
	  default:
		break;
	}
}

extern	PacketDataType
RecvIntegerData(
	NETFILE	*fp,
	int		*val)
{
	PacketDataType type;
	char	buff[SIZE_BUFF];
	
	type = GL_RecvDataType(fp);

	switch	(type) {
	  case	GL_TYPE_INT:
		*val = GL_RecvInt(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = atoi(buff);
		break;
	}
	return type;
}

extern	void
SendBoolData(
	NETFILE			*fp,
	PacketDataType	type,
	Bool			val)
{
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,( val ? "T" : "F" ));
		break;
	  case	GL_TYPE_INT:
		GL_SendInt(fp,(int)val);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,val);
		break;
	  default:
		break;
	}
}

extern	PacketDataType
RecvBoolData(
	NETFILE	*fp,
	Bool	*val)
{
	PacketDataType	type;
	char	buff[SIZE_BUFF];
	
	type = GL_RecvDataType(fp);

	switch	(type) {
	  case	GL_TYPE_INT:
		*val = ( GL_RecvInt(fp) == 0 ) ? FALSE : TRUE;
		break;
	  case	GL_TYPE_BOOL:
		*val = GL_RecvBool(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = (  buff[0]  ==  'T'  ) ? TRUE : FALSE;
		break;
	}
	return	type;
}

extern	void
SendFloatData(
	NETFILE			*fp,
	PacketDataType	type,
	double			val)
{
	char	buff[SIZE_BUFF];
	
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(buff,"%g",val);
		GL_SendString(fp,(char *)buff);
		break;
#if	0
	  case	GL_TYPE_NUMBER:
		xval = DoubleToFixed(val);
		GL_SendFixed(fp,xval);
		FreeFixed(xval);
		break;
#endif
	  case	GL_TYPE_INT:
		GL_SendInt(fp,(int)val);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,val);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,(( val == 0 ) ? FALSE : TRUE ));
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		break;
	}
}

extern	PacketDataType
RecvFloatData(
	NETFILE	*fp,
	double	*val)
{
	PacketDataType	type;
	char	buff[SIZE_BUFF];
	Fixed	*xval;

ENTER_FUNC;
	type = GL_RecvDataType(fp);

	switch	(type)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = atof(buff);
		break;
	  case	GL_TYPE_NUMBER:
		xval = GL_RecvFixed(fp);
		*val = atof(xval->sval) / pow(10.0, xval->slen);
		xfree(xval);
		break;
	  case	GL_TYPE_INT:
		*val = (double)GL_RecvInt(fp);
		break;
	  case	GL_TYPE_FLOAT:
		*val = GL_RecvFloat(fp);
		break;
	}
LEAVE_FUNC;
	return type;
}

extern PacketDataType
RecvBinaryData(NETFILE *fp, LargeByteString *binary)
{
	PacketDataType	type;

ENTER_FUNC;
    type = GL_RecvDataType(fp);
    switch (type) {
    case  GL_TYPE_CHAR:
    case  GL_TYPE_VARCHAR:
    case  GL_TYPE_DBCODE:
    case  GL_TYPE_TEXT:
    case  GL_TYPE_BINARY:
    case  GL_TYPE_BYTE:
    case  GL_TYPE_OBJECT:
        GL_RecvLBS(fp, binary);
        break;
    default:
        Warning("unsupported data type: %d\n", type);
        break;
    }
LEAVE_FUNC;
	return type;
}

extern void
SendBinaryData(
	NETFILE 		*fp, 
	PacketDataType 	type, 
	LargeByteString	*binary)
{
ENTER_FUNC;
    switch (type) {
    case  GL_TYPE_BINARY:
    case  GL_TYPE_BYTE:
    case  GL_TYPE_OBJECT:
		GL_SendDataType(fp, GL_TYPE_OBJECT);
        GL_SendLBS(fp, binary);
        break;
    default:
        Warning("unsupported data type: %d\n", type);
        break;
    }
LEAVE_FUNC;
}
