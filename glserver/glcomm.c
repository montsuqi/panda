/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2004 Ogochan & JMA (Japan Medical Association).

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
#include	<errno.h>
#include	<magic.h>
#include	<string.h>
#include	<strings.h>
#include	<netinet/in.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<math.h>

#include	"types.h"
#include	"libmondai.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"front.h"
#include	"debug.h"

#include	<endian.h>

#define	RECV32(v)	ntohl(v)
#define	RECV16(v)	ntohs(v)
#define	SEND32(v)	htonl(v)
#define	SEND16(v)	htons(v)

#define	GL_OLDTYPE_INT			(PacketDataType)0x10
#define	GL_OLDTYPE_BOOL			(PacketDataType)0x11
#define	GL_OLDTYPE_FLOAT		(PacketDataType)0x20
#define	GL_OLDTYPE_CHAR			(PacketDataType)0x30
#define	GL_OLDTYPE_TEXT			(PacketDataType)0x31
#define	GL_OLDTYPE_VARCHAR		(PacketDataType)0x32
#define	GL_OLDTYPE_BYTE			(PacketDataType)0x40
#define	GL_OLDTYPE_NUMBER		(PacketDataType)0x50
#define	GL_OLDTYPE_DBCODE		(PacketDataType)0x60
#define	GL_OLDTYPE_ARRAY		(PacketDataType)0x90
#define	GL_OLDTYPE_RECORD		(PacketDataType)0xA0

static	LargeByteString	*Buff;

static	struct	magic_set	*Magic;

static	PacketDataType	ToOldType[256];
static	PacketDataType	ToNewType[256];

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c,
	Bool	fNetwork)
{
	nputc(c,fp);
}

extern	PacketClass
GL_RecvPacketClass(
	NETFILE	*fp,
	Bool	fNetwork)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

#define	SendChar(fp,c)	nputc((c),(fp))
#define	RecvChar(fp)	ngetc(fp)

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data,
	Bool	fNetwork)
{
	byte	buff[sizeof(int)];

	if		(  fNetwork  ) {
		*(int *)buff = SEND32(data);
	} else {
		*(int *)buff = data;
	}
	Send(fp,buff,sizeof(int));
}

static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int	data,
	Bool	fNetwork)
{
	byte	buff[sizeof(unsigned int)];

	if		(  fNetwork  ) {
		*(unsigned int *)buff = SEND32(data);
	} else {
		*(unsigned int *)buff = data;
	}
	Send(fp,buff,sizeof(unsigned int));
}

static	unsigned	int
GL_RecvUInt(
	NETFILE	*fp,
	Bool	fNetwork)
{
	byte	buff[sizeof(unsigned int)];
	unsigned	int	data;

	Recv(fp,buff,sizeof(unsigned int));
	if		(  fNetwork  ) {
		data = RECV32(*(unsigned int *)buff);
	} else {
		data = *(unsigned int *)buff;
	}
	return	(data);
}

extern	void
GL_SendLong(
	NETFILE	*fp,
	long	data,
	Bool	fNetwork)
{
	byte	buff[sizeof(long)];

	if		(  fNetwork  ) {
		*(long *)buff = SEND32(data);
	} else {
		*(long *)buff = data;
	}
	Send(fp,buff,sizeof(long));
}

static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data,
	Bool	fNetwork)
{
	byte	buff[sizeof(size_t)];

	if		(  fNetwork  ) {
		*(size_t *)buff = SEND32(data);
	} else {
		*(size_t *)buff = data;
	}
	Send(fp,buff,sizeof(size_t));
}

extern	int
GL_RecvInt(
	NETFILE	*fp,
	Bool	fNetwork)
{
	byte	buff[sizeof(int)];
	int		data;

	Recv(fp,buff,sizeof(int));
	if		(  fNetwork  ) {
		data = RECV32(*(int *)buff);
	} else {
		data = *(int *)buff;
	}
	return	(data);
}

static	size_t
GL_RecvLength(
	NETFILE	*fp,
	Bool	fNetwork)
{
	byte	buff[sizeof(size_t)];
	size_t	data;

	Recv(fp,buff,sizeof(size_t));
	if		(  fNetwork  ) {
		data = RECV32(*(size_t *)buff);
	} else {
		data = *(size_t *)buff;
	}
	return	(data);
}

static	void
GL_RecvLBS(
	NETFILE	*fp,
	LargeByteString	*lbs,
	Bool	fNetwork)
{
	size_t	size;

	size = GL_RecvLength(fp,fNetwork);
	LBS_ReserveSize(lbs,size,FALSE);
	if		(  size  >  0  ) {
		Recv(fp,LBS_Body(lbs),size);
	}
}

static	void
GL_SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs,
	Bool	fNetwork)
{
	GL_SendLength(fp,LBS_Size(lbs),fNetwork);
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
	}
}

extern	void
GL_RecvString(
	NETFILE	*fp,
	char	*str,
	Bool	fNetwork)
{
	size_t	size;

ENTER_FUNC;
	size = GL_RecvLength(fp,fNetwork);
	Recv(fp,str,size);
	str[size] = 0;
LEAVE_FUNC;
}

extern	void
GL_SendString(
	NETFILE	*fp,
	char	*str,
	Bool	fNetwork)
{
	size_t	size;

ENTER_FUNC;
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size,fNetwork);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
LEAVE_FUNC;
}

static	void
GL_SendObject(
	NETFILE	*fp,
	MonObjectType	obj,
	Bool	fNetwork)
{
	unsigned int	iobj;

	iobj = (unsigned int)obj;
	GL_SendUInt(fp,iobj,fNetwork);
}

static	MonObjectType
GL_RecvObject(
	NETFILE	*fp,
	Bool	fNetwork)
{
	return	((MonObjectType)GL_RecvUInt(fp,fNetwork));
}

extern	Fixed	*
GL_RecvFixed(
	NETFILE	*fp,
	Bool	fNetwork)
{
	Fixed	*xval;

ENTER_FUNC;
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp,fNetwork);
	xval->slen = GL_RecvLength(fp,fNetwork);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp,xval->sval,fNetwork);
LEAVE_FUNC;
	return	(xval); 
}

static	void
GL_SendFixed(
	NETFILE	*fp,
	Fixed	*xval,
	Bool	fNetwork)
{
	GL_SendLength(fp,xval->flen,fNetwork);
	GL_SendLength(fp,xval->slen,fNetwork);
	GL_SendString(fp,xval->sval,fNetwork);
}

extern	double
GL_RecvFloat(
	NETFILE	*fp,
	Bool	fNetwork)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

static	void
GL_SendFloat(
	NETFILE	*fp,
	double	data,
	Bool	fNetwork)
{
	Send(fp,&data,sizeof(data));
}

extern	Bool
GL_RecvBool(
	NETFILE	*fp,
	Bool	fNetwork)
{
	char	buf[1];

	Recv(fp,buf,1);
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

static	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data,
	Bool	fNetwork)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
}

static	void
GL_SendDataType(
	NETFILE	*fp,
	PacketClass	c,
	Bool	fNetwork)
{
#ifdef	DEBUG
	printf("SendDataType = %X\n",c);
#endif
	if		(  fFetureOld  ) {
		c = ToOldType[c];
	}
	nputc(c,fp);
}

extern	PacketDataType
GL_RecvDataType(
	NETFILE	*fp,
	Bool	fNetwork)
{
	PacketClass	c;

	c = ngetc(fp);
	if		(  fFetureOld  ) {
		c = ToNewType[c];
	}
	return	(c);
}

static	char	*
ExpandFile(
	char	*cname)
{
	static	char	fname[SIZE_LONGNAME+1];
	char	buff[SIZE_LONGNAME+1];
	char	*PSTOPNGPath = BIN_DIR "/pstopng";
	struct	stat	sb;
	time_t	ps_mtime
		,	tmp_mtime;
	const	char	*type;
	FILE	*fpf;

ENTER_FUNC;
	strcpy(fname,cname);
	if		(  ( type = magic_file(Magic,cname) )  !=  NULL  ) {
		if		(  !strlcmp(type,"PostScript")  ) {
			if	(  !fFeturePS  ) {
				if		(	(  stat(cname,&sb)  ==  0   )
						&&	(  S_ISREG(sb.st_mode)      ) )	{
					ps_mtime = sb.st_mtime;
				} else {
					ps_mtime = 0;
				}
				if		(  fFeturePDF ) {
						sprintf(fname,"%s.pdf",cname); 
				} else {
						sprintf(fname,"%s.png",cname);
				}
				if		(	(  stat(fname,&sb)  ==  0  )
						&&	(  S_ISREG(sb.st_mode)   ) ) {
					tmp_mtime = sb.st_mtime;
				} else {
					tmp_mtime = 0;
					if	(  ( fpf = Fopen(fname,"w") )  !=  NULL  ) {
						fchmod(fileno(fpf), 0600);
						fclose(fpf);	
					}
				}
				if		(  ps_mtime  >  tmp_mtime  ) {
					if		(  fFeturePDF ) {
						sprintf(buff,"ps2pdf13 %s %s", cname,fname);
					} else {
						sprintf(buff,"%s %s > %s",PSTOPNGPath, cname,fname);
					}
					system(buff);
				}
			}
        }
	}
LEAVE_FUNC;
	return	(fname);
}

/*
 *	This function sends value with valiable name.
 */
extern	void
GL_SendValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding,
	Bool		fBlob,
	Bool		fExpand,
	Bool		fNetwork)
{
	int		i;
	struct	stat	sb;
	FILE	*fpf;
	char	*fname;

ENTER_FUNC;
	ValueIsNotUpdate(value);
	GL_SendDataType(fp,ValueType(value),fNetwork);
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		GL_SendInt(fp,ValueInteger(value),fNetwork);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,ValueBool(value),fNetwork);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
		LBS_ReserveSize(Buff,ValueByteLength(value),FALSE);
		memcpy(LBS_Body(Buff),ValueByte(value),ValueByteLength(value));
		GL_SendLBS(fp,Buff,fNetwork);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,ValueToString(value,coding),fNetwork);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,ValueFloat(value),fNetwork);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFixed(fp,&ValueFixed(value),fNetwork);
		break;
	  case	GL_TYPE_OBJECT:
		if		(  fBlob  ) {
			if		(  fExpand  ) {
				fname = ExpandFile(BlobCacheFileName(value));
				if		(  ( fpf = Fopen(fname,"r") )  !=  NULL  ) {
					fstat(fileno(fpf),&sb);
					LBS_ReserveSize(Buff,sb.st_size,FALSE);
					fread(LBS_Body(Buff),sb.st_size,1,fpf);
					fclose(fpf);	
				} else {
					LBS_ReserveSize(Buff,0,FALSE);
				}
				GL_SendLBS(fp,Buff,fNetwork);
			} else {
				GL_SendObject(fp,ValueObject(value),fNetwork);
			}
		}
		break;
	  case	GL_TYPE_ARRAY:
		GL_SendInt(fp,ValueArraySize(value),fNetwork);
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			GL_SendValue(fp,ValueArrayItem(value,i),coding,fBlob,fExpand,fNetwork);
		}
		break;
	  case	GL_TYPE_RECORD:
		GL_SendInt(fp,ValueRecordSize(value),fNetwork);
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			if		(  fFetureOld  ) {
				if		(	(  stricmp(ValueRecordName(value,i),"row")      ==  0  )
						||	(  stricmp(ValueRecordName(value,i),"column")   ==  0  ) )	{
					GL_SendString(fp,ValueRecordName(value,i),fNetwork);
				} else
				if		(  stricmp(ValueRecordName(value,i),"rowattr")  ==  0  ) {
					GL_SendString(fp,"row",fNetwork);
				} else {
					GL_SendString(fp,ValueRecordName(value,i),fNetwork);
					GL_SendValue(fp,ValueRecordItem(value,i),coding,fBlob,fExpand,fNetwork);
				}
			} else {
				GL_SendString(fp,ValueRecordName(value,i),fNetwork);
				GL_SendValue(fp,ValueRecordItem(value,i),coding,fBlob,fExpand,fNetwork);
			}
		}
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

extern	void
GL_RecvValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding,
	Bool		fBlob,
	Bool		fExpand,
	Bool		fNetwork)
{
	PacketDataType	type;
	Fixed		*xval;
	int			ival;
	Bool		bval;
	double		fval;
	char		str[SIZE_BUFF];
	FILE		*fpf;

ENTER_FUNC;
	ValueIsUpdate(value);
	type = GL_RecvDataType(fp,fNetwork);
	ON_IO_ERROR(fp,badio);
	switch	(type)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		dbgmsg("strings");
		GL_RecvString(fp,str,fNetwork);		ON_IO_ERROR(fp,badio);
		SetValueString(value,str,coding);	ON_IO_ERROR(fp,badio);
		break;
	  case	GL_TYPE_NUMBER:
		dbgmsg("NUMBER");
		xval = GL_RecvFixed(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueFixed(value,xval);
		xfree(xval->sval);
		xfree(xval);
		break;
	  case	GL_TYPE_OBJECT:
		dbgmsg("OBJECT");
		if		(  fBlob  ) {
			if		(  fExpand  ) {
				GL_RecvLBS(fp,Buff,fNetwork);
				if		(  ( fpf = Fopen(BlobCacheFileName(value),"w") )  !=  NULL  ) {
					fwrite(LBS_Body(Buff),LBS_Size(Buff),1,fpf);
					fclose(fpf);	
				}
			} else {
				ValueObject(value) = GL_RecvObject(fp,fNetwork);
			}
		}
		break;
	  case	GL_TYPE_INT:
		dbgmsg("INT");
		ival = GL_RecvInt(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueInteger(value,ival);
		break;
	  case	GL_TYPE_FLOAT:
		dbgmsg("FLOAT");
		fval = GL_RecvFloat(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueFloat(value,fval);
		break;
	  case	GL_TYPE_BOOL:
		dbgmsg("BOOL");
		bval = GL_RecvBool(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueBool(value,bval);
		break;
	  default:
		printf("type = [%d]\n",type);
		break;
	}
  badio:
LEAVE_FUNC;
}

extern	void
InitGL_Comm(void)
{
	int		i;

	Buff = NewLBS();
#define	TO_OLDTYPE(t)	ToOldType[GL_TYPE_##t] = GL_OLDTYPE_##t
#define	TO_NEWTYPE(t)	ToNewType[GL_OLDTYPE_##t] = GL_TYPE_##t

	for	( i = 0 ; i < 256 ; i ++ ) {
		ToOldType[i] = GL_TYPE_NULL;
		ToNewType[i] = GL_TYPE_NULL;
	}

	TO_OLDTYPE(INT);
	TO_OLDTYPE(BOOL);
	TO_OLDTYPE(FLOAT);
	TO_OLDTYPE(CHAR);
	TO_OLDTYPE(TEXT);
	TO_OLDTYPE(VARCHAR);
	TO_OLDTYPE(BYTE);
	TO_OLDTYPE(NUMBER);
	TO_OLDTYPE(DBCODE);
	TO_OLDTYPE(ARRAY);
	TO_OLDTYPE(RECORD);

	TO_NEWTYPE(INT);
	TO_NEWTYPE(BOOL);
	TO_NEWTYPE(FLOAT);
	TO_NEWTYPE(CHAR);
	TO_NEWTYPE(TEXT);
	TO_NEWTYPE(VARCHAR);
	TO_NEWTYPE(BYTE);
	TO_NEWTYPE(NUMBER);
	TO_NEWTYPE(DBCODE);
	TO_NEWTYPE(ARRAY);
	TO_NEWTYPE(RECORD);

	if		(  ( Magic = magic_open(MAGIC_SYMLINK|MAGIC_COMPRESS|MAGIC_PRESERVE_ATIME) )
			   ==  NULL  ) {
		fprintf(stderr, "magic: %s\n", strerror(errno));
		exit(1);
	}
	if		(  magic_load(Magic, NULL)  <  0  )	{
		fprintf(stderr, "magic: %s\n", magic_error(Magic));
		exit(1);
	}
}
