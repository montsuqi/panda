/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#define	_HTC_PARSER
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<iconv.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#include	"types.h"
#include	"libmondai.h"
#include	"input.h"
#include	"debug.h"

#ifndef	SIZE_LARGE_BUFF
#define	SIZE_LARGE_BUFF		1024*1024
#endif

extern	char	*
ConvertEncoding(
	char	*tcoding,
	char	*coding,
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
		,	sob;
	char	*ostr;
	char	*cbuff;

	if		(  coding  ==  NULL  ) {
		coding = "utf-8";
	}
	if		(  tcoding  ==  NULL  ) {
		tcoding = "utf-8";
	}
	cd = iconv_open(tcoding,coding);
	sib = strlen(istr);
	sob = sib * 2;
	cbuff = (char *)xmalloc(sob);
	ostr = cbuff;
	iconv(cd,&istr,&sib,&ostr,&sob);
	*ostr = 0;
	iconv_close(cd);
	return	(cbuff);
}

extern	char	*
CheckCoding(
	char	**sstr)
{
	char	*str = *sstr;
	int		blace;
	Bool	fMeta
		,	fHTC
		,	fXML
		,	quote;
	char	*p;
	char	coding[SIZE_LONGNAME+1]
		,	*ret;

ENTER_FUNC;
	blace = 0;
	quote = FALSE;
	fMeta = FALSE;
	fHTC = FALSE;
	fXML = FALSE;
	*coding = 0;
	while	(  *str  !=  0  ) {
		switch	(*str) {
		  case	'<':
			if		(  !strlicmp(str,"<HTC")  ) {
				fHTC = TRUE;
				str += strlen("<HTC");
			} else
			if		(  !strlicmp(str,"<?xml")  ) {
				fXML = TRUE;
				str += strlen("<?xml");
			}
			blace ++;
			break;
		  case	'>':
			blace --;
			if		(  blace  ==  0  ) {
				fMeta = FALSE;
			}
			break;
		  case	'm':
		  case	'M':
			if		(	(  blace  >   0  )
					&&	(  !strlicmp(str,"meta")  ) ) {
				fMeta = TRUE;
				str += strlen("meta");
			}
			break;
		  default:
			break;
		}
		if		(  fMeta  ) {
			if		(  !strlicmp(str,"charset")  ) {
				str += strlen("charset");
				while	(  isspace(*str)  )	str ++;
				if		(  *str  ==  '='  )	str ++;
				while	(  isspace(*str)  )	str ++;
				p = coding;
				while	(	(  !isspace(*str)  )
						&&	(  *str  !=  '"'   ) ) {
					*p ++ = *str ++;
				}
				*p = 0;
				str --;
				goto	quit;
			}
		} else
		if		(  fHTC  ) {
			while	(  isspace(*str)  )	str ++;
			while	(  *str  !=  '>'  ) {
				if		(  !strlicmp(str,"coding")  ) {
					str += strlen("coding");
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '='  )	str ++;
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '"'  )	str ++;
					p = coding;
					while	(	(  !isspace(*str)  )
								&&	(  *str  !=  '"'   ) ) {
						*p ++ = *str ++;
					}
					*p = 0;
				}
				str ++;
			}
			str ++;
			goto	quit;
		} else
		if		(  fXML  ) {
			while	(  *str  !=  '>'  ) {
				if		(  !strlicmp(str,"encoding")  ) {
					str += strlen("encoding");
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '='  )	str ++;
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '"'  )	str ++;
					p = coding;
					while	(	(  !isspace(*str)  )
							&&	(  *str  !=  '"'   ) ) {
						*p ++ = *str ++;
					}
					*p = 0;
				}
				str ++;
			}
			str ++;
			goto	quit;
		}
		str ++;
	}
  quit:
	*sstr = str;
	if		(  *coding  !=  0  ) {
		ret = StrDup(coding);
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	byte	*
GetExecBody(
	char	*command)
{
	LargeByteString	*lbs;
	int		fd
		,	i;
	Bool	fError;
	byte	*ret;
	int		status;
	pid_t	pid;
	int		pResult[2]
		,	pError[2];
	char	buff[SIZE_LARGE_BUFF];
	size_t	size;
	extern	char	**environ;
	char	*argv[4];
	char	*sh;

ENTER_FUNC;
	ret = NULL;
	if		(  command  !=  NULL  ) {
		pipe(pResult);
		pipe(pError);
		if		(  ( pid = fork() )  ==  0  ) {
			dup2(pResult[1],STDOUT_FILENO);
			dup2(pError[1],STDERR_FILENO);
			close(pResult[0]);
			close(pResult[1]);
			close(pError[0]);
			close(pError[1]);
			if		(  ( sh = getenv("SHELL") )  ==  NULL  ) {
				sh = "/bin/sh";
			}
			argv[0] = sh;
			argv[1] = "-c";
			argv[2] = command;
			argv[3] = NULL;
			execve(sh, argv, environ);
		}
		close(pResult[1]);
		close(pError[1]);

		status = 0;
		while( waitpid(pid, &status, WNOHANG) > 0 );

		lbs = NewLBS();
		if		(  WEXITSTATUS(status)  ==  0  ) {
			fd = pResult[0];
			fError = FALSE;
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
		ret = LBS_ToString(lbs);
		FreeLBS(lbs);
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}


static	int
MGETC(
	char	**fp)
{
	int		c;

	c = **fp;
	(*fp) ++;
	return	(c);
}

static	void
MUNGETC(
	int		c,
	char	**fp)
{
	(*fp) --;
	**fp = c;
}

static	void
CopyERuby(
	char	**fp,
	LargeByteString	*lbs)
{
	Bool	fExit;
	int		term
		,	c;

ENTER_FUNC;
	LBS_EmitString(lbs,"<%");
	fExit = FALSE;
	do {
		while	(  isspace(c = MGETC(fp))  ) {
			LBS_Emit(lbs,c);
		}
		switch	(c) {
		  case	'"':
		  case	'\'':
			term = c;
			LBS_Emit(lbs,c);
			while	(  ( c = MGETC(fp) )  !=  term  ) {
				LBS_Emit(lbs,c);
			}
			LBS_Emit(lbs,c);
			break;
		  case	'%':
			if		(  ( c = MGETC(fp) )  ==  '>'  ) {
				fExit = TRUE;
			} else {
				LBS_Emit(lbs,'%');
				MUNGETC(c,fp);
			}
			break;
		  case	'#':
			while	(  c  !=  '\n'  ) {
				LBS_Emit(lbs,c);
				c = MGETC(fp);
			}
			LBS_Emit(lbs,c);
			break;
		  default:
			while	(  !isspace(c)  ) {
				LBS_Emit(lbs,c);
				c = MGETC(fp);
			}
			MUNGETC(c,fp);
			break;
		}
	}	while	(  !fExit  );
	LBS_EmitString(lbs,"%>");
LEAVE_FUNC;
}

static	void
ExecTag(
	char	*tag,
	LargeByteString	*lbs,
	char	**fp)
{
	Bool	fExit;
	int		term
		,	c;

ENTER_FUNC;
	dbgprintf("tag = [%s]",tag);
	LBS_EmitString(lbs,"<");
	LBS_EmitString(lbs,tag);
	fExit = FALSE;
	do {
		while	(  isspace(c = MGETC(fp))  ) {
			LBS_Emit(lbs,c);
		}
		switch	(c) {
		  case	'"':
		  case	'\'':
			term = c;
			LBS_Emit(lbs,c);
			while	(  ( c = MGETC(fp) )  !=  term  ) {
				LBS_Emit(lbs,c);
			}
			LBS_Emit(lbs,c);
			break;
		  case	'>':
			LBS_Emit(lbs,c);
			fExit = TRUE;
			break;
		  default:
			LBS_Emit(lbs,c);
			break;
		}
	}	while	(  !fExit  );
LEAVE_FUNC;
}

static	void
ExecComment(
	LargeByteString	*lbs,
	char	**fp)
{
	Bool	fExit
		,	fComment;
	int		term
		,	c;
	char	buff[SIZE_LONGNAME+1];
	char	*p;
	int		directive;
	char	*work;

#define	DIRECTIVE_NONE		0
#define	DIRECTIVE_INCLUDE	1
#define	DIRECTIVE_EXEC		2

ENTER_FUNC;
	fExit = FALSE;
	fComment = FALSE;
	directive = DIRECTIVE_NONE;
	LBS_EmitString(lbs,"<!--");
	do {
		while	(  isspace(c = MGETC(fp))  ) {
			LBS_Emit(lbs,c);
		}
		if		(	(  !fComment   )
				&&	(  c  ==  '$'  ) ) {
			p = buff;
			while	(  !isspace( c = MGETC(fp) )  ) {
				*p ++ = c;
			}
			*p = 0;
			if		(  strcmp(buff,"include")  ==  0  ) {
				directive = DIRECTIVE_INCLUDE;
				while	(  isspace(c)  )	{
					c = MGETC(fp);
				}
				if		(	(  c  ==  '"'   )
						||	(  c  ==  '\''  ) ) {
					term = c;
					p = buff;
					while	(  ( c = MGETC(fp) )  !=  term  ) {
						*p ++ = c;
					}
					*p = 0;
					LBS_EmitString(lbs,"$include \"");
					LBS_EmitString(lbs,buff);
					LBS_EmitString(lbs,"\"");
				}
			} else
			if		(  strcmp(buff,"exec")  ==  0  ) {
				directive = DIRECTIVE_EXEC;
				while	(  isspace(c)  )	{
					c = MGETC(fp);
				}
				if		(	(  c  ==  '"'   )
						||	(  c  ==  '\''  ) ) {
					term = c;
					p = buff;
					while	(  ( c = MGETC(fp) )  !=  term  ) {
						*p ++ = c;
					}
					*p = 0;
					LBS_EmitString(lbs,"$exec \"");
					LBS_EmitString(lbs,buff);
					LBS_EmitString(lbs,"\"");
				}
			} else {
				LBS_EmitString(lbs,buff);
			}
			fComment = TRUE;
		} else {
			fComment = TRUE;
			switch	(c) {
			  case	'"':
			  case	'\'':
				term = c;
				LBS_Emit(lbs,c);
				while	(  ( c = MGETC(fp) )  !=  term  ) {
					LBS_Emit(lbs,c);
				}
				LBS_Emit(lbs,c);
				break;
			  case	'-':
				if		(  ( c = MGETC(fp) )  ==  '-'  ) {
					if		(  ( c = MGETC(fp) )  ==  '>'  ) {
						fExit = TRUE;
					} else {
						MUNGETC(c,fp);
						MUNGETC('-',fp);
						LBS_Emit(lbs,'-');
					}
				} else {
					MUNGETC(c,fp);
					LBS_Emit(lbs,'-');
				}
				break;
			  default:
				LBS_Emit(lbs,c);
				break;
			}
		}
	}	while	(  !fExit  );
	LBS_EmitString(lbs,"-->");
	switch	(directive) {
	  case	DIRECTIVE_INCLUDE:
		dbgprintf("include \"%s\"",buff);
#if	0
		if		(	(  ( p = strrchr(buff,'.') )  !=  NULL  )
				&&	(  strcmp(p,".xhtc")          ==  0     ) ) {
			IncludeXHTC(lbs,buff);
		} else
#endif
		if		(  ( work = GetFileBody(buff) )  !=  NULL  ) {
			dbgprintf("[%s]",work);
			LBS_EmitString(lbs,work);
			xfree(work);
		}
		break;
	  case	DIRECTIVE_EXEC:
		if		(  ( work = GetExecBody(buff) )  !=  NULL  ) {
			dbgprintf("[%s]",work);
			LBS_EmitString(lbs,work);
			xfree(work);
		}
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

extern	byte	*
GetFileBody(
	char	*fname)
{
	struct	stat	sb;
	byte	*ret;
	int		fd;
	int		c;
	char	**fp
		,	*body
		,	*file
		,	*check;
	LargeByteString	*work;
	char	buff[SIZE_LARGE_BUFF];
	char	*p
		,	*coding;

ENTER_FUNC;
	if		(	(  fname                          !=  NULL  )
			&&	(  ( fd = open(fname,O_RDONLY) )  >=  0     ) ) {
		fstat(fd,&sb);
		if		(  ( p = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0) )
				   !=  NULL  ) {
			file = (byte *)xmalloc(sb.st_size+1);
			memcpy(file,p,sb.st_size);
			munmap(p,sb.st_size);
			file[sb.st_size] = 0;
			close(fd);
			check = file;
			coding = CheckCoding(&check);
			if	   (	(  coding  ==  NULL  )
					||	(	(  stricmp(coding,"utf-8")  !=  0  )
						&&	(  stricmp(coding,"utf8")   !=  0  ) ) ) {
				body = ConvertEncoding("utf-8",coding,file);
				xfree(file);
			} else {
				body = file;
			}
			fp = &body;
			work = NewLBS();
			while	(  ( c = MGETC(fp) )  > 0  ) {
				if		(  c  ==  '<'  ) {
					if		(  ( c = MGETC(fp) )  ==  '%'  ) {
						CopyERuby(fp,work);
					} else {
						while	(  isspace(c)  ) {
							LBS_Emit(work,c);
							c = MGETC(fp);
						}
						p = buff;
						while	(	(  !isspace(c)  )
								&&	(  c  !=  '>'   ) ) {
							*p ++ = c;
							c = MGETC(fp);
						}
						*p = 0;
						MUNGETC(c,fp);
						if		(  strcmp(buff,"!--")  ==  0  ) {
							ExecComment(work,fp);
						} else {
							ExecTag(buff,work,fp);
						}
					}
				} else {
					LBS_Emit(work,c);
				}
			}
			if		(  coding  !=  NULL  ) {
				xfree(coding);
			}
			LBS_EmitEnd(work);
			ret = LBS_ToString(work);
			FreeLBS(work);
		} else {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

