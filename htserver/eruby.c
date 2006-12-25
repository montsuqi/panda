/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2006 Ogochan.
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
#ifdef	USE_ERUBY
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<iconv.h>
#include	<errno.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#include	"types.h"
#include	"libmondai.h"
#include	"HTCparser.h"
#include	"HTClex.h"
#include	"cgi.h"
#include	"exec.h"
#include	"input.h"
#include	"eruby.h"
#include	"debug.h"

#define	ERUBY_TIMEOUT		60

#define	DBIN_FILENO		3
#define	DBOUT_FILENO	4

static	void
ERubyPreambre(
	LargeByteString	*lbs)
{
	LBS_EmitString(lbs,
				   "<%\n"
				   "class MonCore\n"
				   "  def initialize\n"
				   "    @fpDBR = IO.new(3,\"r\")\n"
				   "    @fpDBW = IO.new(4,\"w\")\n"
				   "    @values = Hash.new\n"
				   "  end\n"
				   "  def decode(string)\n"
				   "    if  string\n"
				   "      string.tr('+', ' ').gsub(/((?:%[0-9a-fA-F]{2})+)/n) do\n"
				   "        [$1.delete('%')].pack('H*')\n"
				   "      end\n"
				   "    else\n"
				   "	  \"\"\n"
				   "    end\n"
				   "  end\n"
				   "  def encode(val)\n"
				   "    if  val\n"
				   "      if val.class == String\n"
				   "        string = val\n"
				   "      else\n"
				   "        string = val.to_s\n"
				   "      end\n"
				   "      string.gsub(/([^ a-zA-Z0-9_.-]+)/n) do\n"
				   "        '%' + $1.unpack('H2' * $1.size).join('%').upcase\n"
				   "      end.tr(' ', '+')\n"
				   "    else\n"
				   "      \"\"\n"
				   "    end\n"
				   "  end\n"
				   "  def []=(name,value)\n"
				   "    @values[name] = value\n"
				   "    @fpDBW.printf(\"%s:\\n\",name)\n"
				   "    @fpDBW.printf(\"%s\\n\",encode(value))\n"
				   "    @fpDBW.flush\n"
				   "  end\n"
				   "  def [](name)\n"
				   "    if @values[name]\n"
				   "      @values[name]\n"
				   "    else\n"
				   "      @fpDBW.printf(\"%s\\n\",name)\n"
				   "      @fpDBW.flush\n"
				   "      value = decode(@fpDBR.gets.chop)\n"
				   "      @values[name] = value\n"
				   "    end\n"
				   "  end\n"
				   "  def close\n"
				   "    @fpDBW.printf(\"\")\n"
				   "    @fpDBW.close\n"
				   "  end\n"
				   "end\n"
				   "hostvalue = MonCore.new\n"
				   "%>\n");
}

static	void
ERubyPostambre(
	LargeByteString	*lbs)
{
	LBS_EmitString(lbs,
				   "<%\n"
				   "hostvalue.close\n"
				   "%>\n");
}

#define	SIZE_LARGE_BUFF		1024*1024

static	void
DataProcess(
	int		ifd,
	int		ofd,
	pid_t	pid)
{
	char	name[SIZE_LONGNAME+1]
		,	data[SIZE_LARGE_BUFF];
	byte	value[SIZE_LARGE_BUFF];
	byte	*got
		,	*p;
	FILE	*fpR
		,	*fpW;
	fd_set	ready;
	int		maxfd;
	int		ret;
	struct	timeval	timeout;

ENTER_FUNC;
	fpR = fdopen(ifd,"r");
	fpW = fdopen(ofd,"w");
	maxfd = ifd;
	while	(  TRUE  )	{
		timeout.tv_sec = ERUBY_TIMEOUT;
		timeout.tv_usec = 0;
		FD_ZERO(&ready);
		FD_SET(ifd,&ready);
		ret = select(maxfd+1,&ready,NULL,NULL,&timeout);
        if		(  ret  <  0  )	{
            if (errno == EINTR)
                continue;
            Error("select: ", strerror(errno));
        }
		if		(  FD_ISSET(ifd,&ready)  ) {
			if		(  fgets(name,SIZE_LONGNAME,fpR)  ==  NULL  )	break;
			StringChop(name);
			if		(  strlen(name)  ==  0  )	break;
			if		(  ( p = strchr(name,':') )  !=  NULL  ) {
				*p = 0;
				fgets(data,SIZE_LARGE_BUFF,fpR);
				DecodeStringURL(value,data);
				SaveValue(name,value,FALSE);
			} else {
				dbgprintf("request [%s]",name);
				got = GetHostValue(name,FALSE);
				EncodeStringURL(data,got);
				fprintf(fpW,"%s\n",data);
				dbgprintf("reply [%s]",data);
				fflush(fpW);
			}
		} else {
			kill(pid,SIGKILL);
			break;
		}
	}
	fclose(fpR);
	fclose(fpW);
LEAVE_FUNC;
}

static	void
MakeErrorBody(
	LargeByteString	*lbs,
	char	*p)
{
	byte	buff[SIZE_LARGE_BUFF];
	char	*q;
	Bool	fBody;
	int		i;

	fBody = FALSE;
	i = 0;
	while	(  *p  !=  0  ) {
		if		(  ( q = strchr(p,'\n') )  !=  NULL  ) {
			*q = 0;
		}
		if		(  fBody  ) {
			sprintf(buff,"%5d:%s\n",i,p);
		} else {
			if		(  strlcmp(p,"---")  ==  0  )	fBody = TRUE;
			sprintf(buff,"%s\n",p);
			i = 0;
		}
		EmitWithEscape(lbs,buff,TRUE);
		if		(  q  ==  NULL  )	break;
		p = q + 1;
		i ++;
	}
}

static	HTCInfo	*
MakeErrorReport(
	char	*str)
{
	HTCInfo	*ret;

ENTER_FUNC;
	ret = NewHTCInfo();
	ret->code = NewLBS();
	LBS_EmitString(ret->code,
				   "<html><head>\n"
				   "<meta http-equiv=\"Content-Type\" content=\"text/html;"
				   "charset=utf-8\">"
				   "<title>htserver error</title>"
				   "</head><body>\n"
				   "<H1>eRuby error</H1>\n"
				   "<hr>\n"
				   "<pre>\n");
	MakeErrorBody(ret->code,str);
	LBS_EmitString(ret->code,
				   "</pre>\n");
LEAVE_FUNC;
	return	(ret);
}

static	HTCInfo	*
ParseFile(
	char	*fname,
	Bool	fHTC)
{
	HTCInfo	*ret;
	byte	*str;
	int		fd
		,	i;
	int		pSource[2]
		,	pResult[2]
		,	pError[2]
		,	pDBR[2]
		,	pDBW[2];
	LargeByteString	*lbs;
	char	buff[SIZE_LARGE_BUFF];
	size_t	size;
	Bool	fChange
		,	fError;
	char	*coding
		,	*istr
		,	*ostr;
	int		status;
	pid_t	pid;

ENTER_FUNC;
	if		(  ( str = GetFileBody(fname) )  !=  NULL  ) {
		pipe(pSource);
		pipe(pResult);
		pipe(pError);
		pipe(pDBR);
		pipe(pDBW);
		if		(  ( pid = fork() )  ==  0  ) {
			dup2(pSource[0],STDIN_FILENO);
			dup2(pResult[1],STDOUT_FILENO);
			dup2(pError[1],STDERR_FILENO);
			close(pSource[0]);
			close(pSource[1]);
			close(pResult[0]);
			close(pResult[1]);
			close(pError[0]);
			close(pError[1]);
			dup2(pDBW[0],DBIN_FILENO);
			dup2(pDBR[1],DBOUT_FILENO);
			close(pDBW[0]);
			close(pDBW[1]);
			close(pDBR[0]);
			close(pDBR[1]);
			execl(ERUBY_PATH,"eruby","-Mf","-Ku",NULL);
		}
		close(pSource[0]);
		close(pResult[1]);
		close(pError[1]);
		close(pDBR[1]);
		close(pDBW[0]);
		istr = str;
		coding = CheckCoding(&istr);
		if	   (	(  coding  ==  NULL  )
				||	(	(  stricmp(coding,"utf-8")  !=  0  )
					&&	(  stricmp(coding,"utf8")   !=  0  ) ) ) {
			ostr = ConvertEncoding("utf-8",coding,istr);
			xfree(str);
			str = ostr;
			fChange = TRUE;
		} else {
			fChange = FALSE;
		}
		lbs = NewLBS();
		ERubyPreambre(lbs);
		write(pSource[1],LBS_Body(lbs),LBS_Size(lbs));
#ifdef	DEBUG
		printf("code:-------\n");
		printf("%s",(char *)LBS_Body(lbs));
		printf("%s",str);
		printf("------------\n");
#endif
		write(pSource[1],str,strlen(str));
		LBS_EmitStart(lbs);
		ERubyPostambre(lbs);
		write(pSource[1],LBS_Body(lbs),LBS_Size(lbs));
		FreeLBS(lbs);
		close(pSource[1]);
		DataProcess(pDBR[0],pDBW[1],pid);
		xfree(str);
#if	1
		//while( waitpid(pid, &status, WNOHANG) > 0 );
		waitpid(pid, &status, 0);
#else
		(void)wait(&status);
#endif

		lbs = NewLBS();
		if		(  WEXITSTATUS(status)  ==  0  ) {
			fd = pResult[0];
			fError = FALSE;
			if		(	(  fHTC     )
					&&	(  fChange  ) ) {
				sprintf(buff,"<htc coding=\"%s\">",coding);
				LBS_EmitString(lbs,buff);
			}
		} else {
			sprintf(buff,"code = %d\n",(int)WEXITSTATUS(status));
			LBS_EmitString(lbs,buff);
			fError = TRUE;
			fd = pError[0];
		}
		while	(  ( size = read(fd,buff,SIZE_LARGE_BUFF) )  >  0  ) {
			for	( i = 0 ; i < size ; i ++ ) {
				LBS_Emit(lbs,buff[i]);
			}
		}
		close(pResult[0]);
		close(pError[0]);
		str = LBS_ToString(lbs);
		FreeLBS(lbs);
		if		(  fError  ) {
			ret = MakeErrorReport(str);
		} else {
			if		(  fChange  ) {
				ostr = ConvertEncoding(coding,"utf-8",str);
				xfree(str);
				str = ostr;
			}
			if		(  fHTC  ) {
				HTC_FileName = fname;
				HTC_cLine = 1;
				HTC_Memory = str;
				_HTC_Memory = str;
				if		(  fDump  ) {
					SaveValue("_eruby_output",str,FALSE);
				}
				ret = HTCParserCore();
			} else {
				ret = NewHTCInfo();
				ret->code = NewLBS();
				LBS_EmitString(ret->code,str);
			}
		}
		if		(  coding  !=  NULL  ) {
			xfree(coding);
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	HTCInfo	*
RHTMLParseFile(
	char	*fname)
{
	HTCInfo	*ret;

ENTER_FUNC;
	ret = ParseFile(fname,FALSE);
LEAVE_FUNC;
	return	(ret);
}

extern	HTCInfo	*
RHTCParseFile(
	char	*fname)
{
	HTCInfo	*ret;

ENTER_FUNC;
	ret = ParseFile(fname,TRUE);
LEAVE_FUNC;
	return	(ret);
}

#endif
