/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"const.h"
#include	"enum.h"
#include	"wfc.h"
#include	"DBparser.h"
#include	"directory.h"
#include	"driver.h"
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fNoConv;
static	Bool	fNoFiller;
static	Bool	fFiller;
static	Bool	fSPA;
static	Bool	fLinkage;
static	Bool	fScreen;
static	Bool	fWindowPrefix;
static	Bool	fRecordPrefix;
static	Bool	fLDW;
static	Bool	fLDR;
static	Bool	fDB;
static	Bool	fDBREC;
static	Bool	fDBCOMM;
static	Bool	fDBPATH;
static	Bool	fMCP;
static	Bool	fFull;
static	int		TextSize;
static	int		ArraySize;
static	char	*Prefix;
static	char	*RecName;
static	char	*LD_Name;
static	char	*BD_Name;
static	char	*Directory;
static	char	*Lang;

static	CONVOPT	*Conv;

static	int		level;
static	int		Col;
static	int		is_return = FALSE;
static	char	namebuff[SIZE_BUFF];

static	void	_COBOL(CONVOPT *conv, ValueStruct *val);

static	void
PutLevel(
	int		level,
	Bool	fNum)
{
	int		n;

	n = 8;
	if		(  level  >  1  ) {
		n += 4 + ( level - 2 ) * 2;
	}
	for	( ; n > 0 ; n -- ) {
		fputc(' ',stdout);
	}
	if		(  fNum  ) {
		printf("%02d  ",level);
	} else {
		printf("    ");
	}
	Col = 0;
}

static	void
PutTab(
	int		unit)
{
	int		n;

	n = unit - ( Col % unit );
	Col += n;
	for	( ; n > 0 ; n -- ) {
		fputc(' ',stdout);
	}
}

static	void
PutChar(
	int		c)
{
	fputc(c,stdout);
	Col ++;
}

static	void
PutString(
	char	*str)
{
	int		c;

	while	(  *str  !=  0  ) {
		if		(  fNoConv  ) {
			PutChar(*str);
		} else {
			c = toupper(*str);
			if		(  c  ==  '_'  ) {
				c = '-';
			}
			PutChar(c);
		}
		str ++;
	}
}

static	void
PutName(
	char	*name)
{
	char	buff[SIZE_BUFF];

	if		(	(  name           ==  NULL  )
			||	(  !stricmp(name,"filler")  ) ) {
		strcpy(buff,"filler");
	} else {
		int threshold;

		sprintf(buff,"%s%s",Prefix,name);
#define MAX_CHARACTER_IN_LINE 65

		threshold = MAX_CHARACTER_IN_LINE -
					(8 + level * 2 + 4 + 4 + 18 + 2 + 18 + 1);
		if		(	(  threshold <= 0          )
				||	(  strlen(buff) > threshold) ) {
			is_return = TRUE;
		}
	}
	PutString(buff);
}

static	char	PrevName[SIZE_BUFF];
static	int		PrevCount;
static	char	*PrevSuffix[] = { "X","Y","Z" };
static	void
_COBOL(
	CONVOPT		*conv,
	ValueStruct	*val)
{
	int		i
	,		n;
	ValueStruct	*tmp
	,			*dummy;
	char	buff[SIZE_BUFF+1];
	char	*name;

	if		(  val  ==  NULL  )	return;

	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		PutString("PIC S9(9)   BINARY");
		break;
	  case	GL_TYPE_FLOAT:
		PutString("PIC X(8)");
		break;
	  case	GL_TYPE_BOOL:
		PutString("PIC X");
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		sprintf(buff,"PIC X(%d)",ValueStringLength(val));
		PutString(buff);
		break;
	  case	GL_TYPE_OBJECT:
		sprintf(buff,"PIC X(%d)",sizeof(int)+SIZE_OID);
		PutString(buff);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  ValueFixedSlen(val)  ==  0  ) {
			sprintf(buff,"PIC S9(%d)",ValueFixedLength(val));
		} else {
			sprintf(buff,"PIC S9(%d)V9(%d)",
					(ValueFixedLength(val) - ValueFixedSlen(val)),
					ValueFixedSlen(val));
		}
		PutString(buff);
		break;
	  case	GL_TYPE_TEXT:
		sprintf(buff,"PIC X(%d)",conv->textsize);
		PutString(buff);
		break;
	  case	GL_TYPE_ARRAY:
		tmp = ValueArrayItem(val,0);
		n = ValueArraySize(val);
		if		(  n  ==  0  ) {
			n = conv->arraysize;
		}
		switch	(ValueType(tmp)) {
		  case	GL_TYPE_RECORD:
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
			_COBOL(conv,tmp);
			break;
		  case	GL_TYPE_ARRAY:
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
			dummy = NewValue(GL_TYPE_RECORD);
			sprintf(buff,"%s-%s",PrevName,PrevSuffix[PrevCount]);
			ValueAddRecordItem(dummy,buff,tmp);
			PrevCount ++;
			_COBOL(conv,dummy);
			break;
		  default:
			_COBOL(conv,tmp);
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
			break;
		}
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		if		(  fFull  ) {
			name = namebuff + strlen(namebuff);
		} else {
			name = namebuff;
		}
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			printf(".\n");
			PutLevel(level,TRUE);
			if		(  fFull  )	{
				sprintf(name,"%s%s",(( *namebuff == 0 ) ? "": "-"),
						ValueRecordItem(val,i));
			} else {
				strcpy(namebuff,ValueRecordName(val,i));
			}
			strcpy(PrevName,ValueRecordName(val,i));
			PrevCount = 0;
			PutName(namebuff);
			tmp = ValueRecordItem(val,i);
			if		(  is_return  ) {
				/* new line if current ValueStruct children is item and it has
				   occurs */
				if		(	(  ValueType(tmp)  !=  GL_TYPE_RECORD  )
						&&	(  ValueType(tmp)  ==  GL_TYPE_ARRAY  )
						&&	(  ValueType(ValueArrayItem(tmp,0))  != GL_TYPE_RECORD  ) ) {
					printf("\n");
					PutLevel(level,FALSE);
				}
			  	is_return = FALSE;
			}
			if		(  ValueType(tmp)  !=  GL_TYPE_RECORD  ) {
				PutTab(4);
			}
			_COBOL(conv,tmp);
			*name = 0;
		}
		level --;
		break;
	  case	GL_TYPE_ALIAS:
	  default:
		break;
	}
}

static	void
COBOL(
	CONVOPT		*conv,
	ValueStruct	*val)
{
	*namebuff = 0;
	_COBOL(conv,val);
}

static	void
SIZE(
	CONVOPT		*conv,
	ValueStruct	*val)
{
	char	buff[SIZE_BUFF+1];

	if		(  val  ==  NULL  )	return;
	level ++;
	PutLevel(level,TRUE);
	PutName("filler");
	PutTab(8);
	sprintf(buff,"PIC X(%d)",SizeValue(conv,val));
	PutString(buff);
	level --;
}

static	void
MakeFromRecord(
	char	*name)
{
	RecordStruct	*rec;

dbgmsg(">MakeFromRecord");
	level = 1;
	DD_ParserInit();
	DB_ParserInit();
	if		(  ( rec = DB_Parser(name) )  !=  NULL  ) {
		PutLevel(level,TRUE);
		if		(  *RecName  ==  0  ) {
			PutString(ValueName);
		} else {
			PutString(RecName);
		}
		if		(  fFiller  ) {
			printf(".\n");
			SIZE(Conv,rec->value);
		} else {
			COBOL(Conv,rec->value);
		}
		printf(".\n");
	}
dbgmsg("<MakeFromRecord");
}

static	void
MakeLD(void)
{
	LD_Struct	*ld;
	int		i;
	char	buff[SIZE_BUFF+1];
	char	*_prefix;
	size_t	size
	,		num
	,		spasize
	,		mcpsize;
	int		base;

dbgmsg(">MakeLD");
	InitDirectory();
	SetUpDirectory(Directory,LD_Name,"","");
	if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
		Error("LD not found.\n");
	}

	PutLevel(1,TRUE);
	if		(  *RecName  ==  0  ) {
		sprintf(buff,"%s",ld->name);
	} else {
		sprintf(buff,"%s",RecName);
	}
	PutString(buff);
	printf(".\n");

	ConvSetSize(Conv,ld->textsize,ld->arraysize);

	base = 2;
	if		(	(  fLDR     )
			||	(  fLDW     ) ) {
		size =	SizeValue(Conv,ThisEnv->mcprec->value);
			+	ThisEnv->linksize
			+	SizeValue(Conv,ld->sparec->value);
		for	( i = 0 ; i < ld->cWindow ; i ++ ) {
			size += SizeValue(Conv,ld->window[i]->rec->value);
		}
		num = ( size / SIZE_BLOCK ) + 1;

		PutLevel(base,TRUE);
		PutName("dataarea");
		printf(".\n");

		PutLevel(base+1,TRUE);
		PutName("data");
		PutTab(12);
		printf("OCCURS  %d.\n",num);

		PutLevel(base+2,TRUE);
		PutName("filler");
		PutTab(12);
		printf("PIC X(%d).\n",SIZE_BLOCK);

		PutLevel(base,TRUE);
		PutName("inputarea-red");
		PutTab(4);
		printf("REDEFINES   ");
		PutName("dataarea.\n");
		base ++;
	} else {
		num = 0;
	}
	if		(  fMCP  ) {
		if		(  ( mcpsize = SizeValue(Conv,ThisEnv->mcprec->value) )
				   >  0  ) {
			PutLevel(base,TRUE);
			PutName("mcpdata");
			level = base;
			if		(	(  fFiller  )
					||	(  fLDW     ) ) {
				printf(".\n");
				SIZE(Conv,ThisEnv->mcprec->value);
			} else {
				_prefix = Prefix;
				Prefix = "ldr-mcp-";
				COBOL(Conv,ThisEnv->mcprec->value);
				Prefix = _prefix;
			}
			printf(".\n");
		}
	}
	if		(  fSPA  ) {
		if		(  ( spasize = SizeValue(Conv,ld->sparec->value) )
				   >  0  ) {
			PutLevel(base,TRUE);
			PutName("spadata");
			if		(  ld->sparec  ==  NULL  ) {
				printf(".\n");
				PutLevel(base+1,TRUE);
				PutName("filler");
				printf("      PIC X(%d).\n",spasize);
			} else {
				level = base;
				if		(	(  fFiller  )
						||	(  fLDW     ) ) {
					printf(".\n");
					SIZE(Conv,ld->sparec->value);
				} else {
					_prefix = Prefix;
					Prefix = "spa-";
					COBOL(Conv,ld->sparec->value);
					Prefix = _prefix;
				}
				printf(".\n");
			}
		}
	}
	if		(  fLinkage  ) {
		if		(  SizeValue(Conv,ThisEnv->linkrec->value)  >  0  ) {
			PutLevel(base,TRUE);
			PutName("linkdata");
			level = base;
			if		(	(  fFiller  )
					||	(  fLDR     )
					||	(  fLDW     ) ) {
				printf(".\n");
				PutLevel(level+1,TRUE);
				PutName("filler");
				printf("      PIC X(%d)",ThisEnv->linksize);
			} else {
				_prefix = Prefix;
				Prefix = "lnk-";
				COBOL(Conv,ThisEnv->linkrec->value);
				Prefix = _prefix;
			}
			printf(".\n");
		}
	}
	if		(  fScreen  ) {
		PutLevel(base,TRUE);
		sprintf(buff,"screendata");
		PutName(buff);
		printf(".\n");
		_prefix = Prefix;
		for	( i = 0 ; i < ld->cWindow ; i ++ ) {
			if		(  SizeValue(Conv,ld->window[i]->rec->value)  >  0  ) {
				Prefix = _prefix;
				PutLevel(base+1,TRUE);
				sprintf(buff,"%s",ld->window[i]->name);
				PutName(buff);
				if		(  fWindowPrefix  ) {
					sprintf(buff,"%s-",ld->window[i]->name);
					Prefix = StrDup(buff);
				}
				level = base+1;
				if		(	(  fFiller  )
						||	(  fLDR     )
						||	(  fLDW     ) ) {
					printf(".\n");
					SIZE(Conv,ld->window[i]->rec->value);
				} else {
					COBOL(Conv,ld->window[i]->rec->value);
				}
				printf(".\n");
				if		(  fWindowPrefix  ) {
					xfree(Prefix);
				}
			}
		}
	}
	if		(	(  fLDR     )
			||	(  fLDW     ) ) {
		PutLevel(1,TRUE);
		PutName("blocks");
		PutTab(8);
		printf("PIC S9(9)   BINARY  VALUE   %d.\n",num);
	}
dbgmsg("<MakeLD");
}

static	void
MakeLinkage(void)
{
	LD_Struct	*ld;
	char	*_prefix;

	InitDirectory();
	SetUpDirectory(Directory,LD_Name,"","");
	if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
		Error("LD not found.\n");
	}

	ConvSetSize(Conv,ld->textsize,ld->arraysize);

	_prefix = Prefix;
	Prefix = "";
	PutLevel(1,TRUE);
	PutName("linkarea.\n");
	PutLevel(2,TRUE);
	PutName("linkdata-redefine.\n");
	PutLevel(3,TRUE);
	PutName("filler");
	printf("      PIC X(%d).\n",ThisEnv->linksize);

	PutLevel(2,TRUE);
	PutName(ld->name);
	PutTab(4);
	printf("REDEFINES   ");
	PutName("linkdata-redefine");
	Prefix = _prefix;
	level = 3;
	COBOL(Conv,ThisEnv->linkrec->value);
	printf(".\n");
}

static	void
MakeDB(void)
{
	size_t	msize
	,		size;
	int		i;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
		exit(1);
	}

	ConvSetSize(Conv,textsize,arraysize);

	PutLevel(1,TRUE);
	PutName("dbarea");
	printf(".\n");
	msize = 0;
	for	( i = 1 ; i < cDB ; i ++ ) {
		size = SizeValue(Conv,dbrec[i]->value);
		msize = ( msize > size ) ? msize : size;
	}

	PutLevel(2,TRUE);
	PutName("dbdata");
	PutTab(12);
	printf("PIC X(%d).\n",msize);
}

static	void
PutDBREC(
	ValueStruct	*value,
	char	*rname,
	size_t	msize)
{
	size_t	size;
	char	*_prefix;
	char	prefix[SIZE_LONGNAME+1];

ENTER_FUNC;
	level = 1;
	PutLevel(level,TRUE);
	if		(  fRecordPrefix  ) {
		sprintf(prefix,"%s-",rname);
		Prefix = prefix;
	}
	_prefix = Prefix;
	Prefix = "";
	PutName(rname);
	Prefix = _prefix;
	COBOL(Conv,value);
	printf(".\n");

	if		(  !fNoFiller  ) {
		size = SizeValue(Conv,value);
		if		(  msize  !=  size  ) {
			PutLevel(2,TRUE);
			PutName("filler");
			PutTab(12);
			printf("PIC X(%d).\n",msize - size);
		}
	}
LEAVE_FUNC;
}

static	void
MakeDBREC(
	char	*name)
{
	size_t	msize
	,		size;
	int		i
		,	j
		,	k;
	char	rname[SIZE_LONGNAME+1];
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec
		,			*rec;
	DB_Struct		*db;
	PathStruct		*path;
	DB_Operation	*op;
	ValueStruct		*value;
	GHashTable		*dbtable;
	size_t			arraysize
	,				textsize;
	size_t	cDB;
	char			*p;
	int				rno;

dbgmsg(">MakeDBREC");
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
		dbtable = ld->DB_Table;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
		dbtable = bd->DB_Table;
	} else {
		Error("LD or BD not specified");
		exit(1);
	}

	ConvSetSize(Conv,textsize,arraysize);
	msize = 64;
	for	( i = 1 ; i < cDB ; i ++ ) {
		rec = dbrec[i];
		size = SizeValue(Conv,rec->value);
		msize = ( msize > size ) ? msize : size;
		for	( j = 0 ; j < rec->opt.db->pcount ; j ++ ) {
			path = rec->opt.db->path[j];
			if		(  path->args  !=  NULL  ) {
				size = SizeValue(Conv,path->args);
				msize = ( msize > size ) ? msize : size;
			}
			for	( k = 0 ; k < path->ocount ; k ++ ) {
				op = path->ops[k];
				if		(  op->args  !=  NULL  ) {
					size = SizeValue(Conv,op->args);
					msize = ( msize > size ) ? msize : size;
				}
			}
		}
	}
	if		( ( p = strchr(name,'.') )  !=  NULL  ) {
		*p = 0;
	}
	if		(  ( rno = (int)g_hash_table_lookup(dbtable,name) )  !=  0  ) {
		rec = dbrec[rno-1];
		if		(  *RecName  ==  0  ) {
			strcpy(rname,rec->name);
		} else {
			strcpy(rname,RecName);
		}
		PutDBREC(rec->value,rname,msize);
		for	( j = 0 ; j < rec->opt.db->pcount ; j ++ ) {
			path = rec->opt.db->path[j];
			if		(  path->args  !=  NULL  ) {
				if		(  *RecName  ==  0  ) {
					sprintf(rname,"%s-%s",rec->name,path->name);
				} else {
					sprintf(rname,"%s-%s",RecName,path->name);
				}
				PutDBREC(path->args,rname,msize);
			}
			for	( k = 0 ; k < path->ocount ; k ++ ) {
				op = path->ops[k];
				if		(  op->args  !=  NULL  ) {
					if		(  *RecName  ==  0  ) {
						sprintf(rname,"%s-%s-%s",rec->name,path->name,op->name);
					} else {
						sprintf(rname,"%s-%s-%s",RecName,path->name,op->name);
					}
					PutDBREC(op->args,rname,msize);
				}
			}
		}
	}
dbgmsg("<MakeDBREC");
}

static	void
MakeDBCOMM(void)
{
	size_t	msize
	,		size
	,		num;
	int		i;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
		exit(1);
	}

	ConvSetSize(Conv,textsize,arraysize);

	msize = 0;
	for	( i = 1 ; i < cDB ; i ++ ) {
		size = SizeValue(Conv,dbrec[i]->value);
		msize = ( msize > size ) ? msize : size;
	}

	PutLevel(1,TRUE);
	PutName("dbcomm");
	printf(".\n");

	num = ( ( msize + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;

	PutLevel(2,TRUE);
	PutName("dbcomm-blocks");
	printf(".\n");

	PutLevel(3,TRUE);
	PutName("dbcomm-block");
	PutTab(12);
	printf("OCCURS  %d.\n",num);

	PutLevel(4,TRUE);
	PutName("filler");
	PutTab(12);
	printf("PIC X(%d).\n",SIZE_BLOCK);

	PutLevel(2,TRUE);
	PutName("dbcomm-data");
	PutTab(4);
	printf("REDEFINES   ");
	PutName("dbcomm-blocks");
	printf(".\n");

	PutLevel(3,TRUE);
	PutName("dbcomm-ctrl");
	printf(".\n");

	PutLevel(4,TRUE);
	PutName("dbcomm-func");
	PutTab(12);
	printf("PIC X(16).\n");

	PutLevel(4,TRUE);
	PutName("dbcomm-rc");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(4,TRUE);
	PutName("dbcomm-path");
	printf(".\n");

	PutLevel(5,TRUE);
	PutName("dbcomm-path-blocks");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(5,TRUE);
	PutName("dbcomm-path-rname");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(5,TRUE);
	PutName("dbcomm-path-pname");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(3,TRUE);
	PutName("dbcomm-record");
	printf(".\n");

	PutLevel(4,TRUE);
	PutName("filler");
	PutTab(12);
	printf("PIC X(%d).\n",msize);
}

static	void
MakeDBPATH(void)
{
	int		i
	,		j
	,		blocks;
	size_t	size;
	char	buff[SIZE_NAME*2+1];
	DB_Struct	*db;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

dbgmsg(">MakeDBPATH");
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
			exit(1);
		} else {
			dbrec = ld->db;
			arraysize = ld->arraysize;
			textsize = ld->textsize;
			cDB = ld->cDB;
		}
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
			exit(1);
		} else {
			dbrec = bd->db;
			arraysize = bd->arraysize;
			textsize = bd->textsize;
			cDB = bd->cDB;
		}
	} else {
		Error("LD or BD not specified");
		exit(1);
	}

	ConvSetSize(Conv,textsize,arraysize);

	PutLevel(1,TRUE);
	PutName("dbpath");
	printf(".\n");
	for	( i = 1 ; i < cDB ; i ++ ) {
		db = dbrec[i]->opt.db;
		size = SizeValue(Conv,dbrec[i]->value);
		blocks = ( ( size + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;
		
		for	( j = 0 ; j < db->pcount ; j ++ ) {
			PutLevel(2,TRUE);
			sprintf(buff,"path-%s-%s",dbrec[i]->name,db->path[j]->name);
			PutName(buff);
			printf(".\n");

			PutLevel(3,TRUE);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",blocks);

			PutLevel(3,TRUE);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",i);

			PutLevel(3,TRUE);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",j);
		}
	}
dbgmsg("<MakeDBPATH");
}

static	void
MakeMCP(void)
{
ENTER_FUNC;
	InitDirectory();
	SetUpDirectory(Directory,"","","");

	Prefix = "";
	PutLevel(1,TRUE);
	PutName("mcparea");
	Prefix = "mcp_";
	level = 1;
	COBOL(Conv,ThisEnv->mcprec->value);
	printf(".\n");
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"ldw",	BOOLEAN,	TRUE,		(void*)&fLDW,
		"制御フィールド以外をFILLERにする"				},
	{	"ldr",	BOOLEAN,	TRUE,		(void*)&fLDR,
		"制御フィールドとSPA以外をFILLERにする"			},
	{	"spa",		BOOLEAN,	TRUE,	(void*)&fSPA,
		"SPA領域を出力する"								},
	{	"linkage",	BOOLEAN,	TRUE,	(void*)&fLinkage,
		"連絡領域を出力する"							},
	{	"screen",	BOOLEAN,	TRUE,	(void*)&fScreen,
		"画面レコード領域を出力する"					},
	{	"dbarea",	BOOLEAN,	TRUE,	(void*)&fDB,
		"MCPSUB用領域を出力する"						},
	{	"dbrec",	BOOLEAN,	TRUE,	(void*)&fDBREC,
		"MCBSUBの引数に使うレコード領域を出力する"	},
	{	"dbpath",	BOOLEAN,	TRUE,	(void*)&fDBPATH,
		"DBのパス名テーブルを出力する"					},
	{	"dbcomm",	BOOLEAN,	TRUE,	(void*)&fDBCOMM,
		"MCPSUBとDBサーバとの通信領域を出力する"		},
	{	"mcp",		BOOLEAN,	TRUE,	(void*)&fMCP,
		"MCPAREAを出力する"								},
	{	"lang",		STRING,		TRUE,	(void*)&Lang	,
		"対象言語名"			 						},
	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		"textの最大長"									},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		"可変要素配列の最大繰り返し数"					},
	{	"prefix",	STRING,		TRUE,	(void*)&Prefix,
		"項目名の前に付加する文字列"					},
	{	"wprefix",	BOOLEAN,	TRUE,	(void*)&fWindowPrefix,
		"画面レコードの項目の前にウィンドウ名を付加する"},
	{	"rprefix",	BOOLEAN,	TRUE,	(void*)&fRecordPrefix,
		"データベースレコードの項目の前にレコード名を付加する"},
	{	"name",		STRING,		TRUE,	(void*)&RecName,
		"レコードの名前"								},
	{	"filler",	BOOLEAN,	TRUE,	(void*)&fFiller,
		"レコードの中身をFILLERにする"					},
	{	"full",		BOOLEAN,	TRUE,	(void*)&fFull,
		"階層構造を名前に反映する"						},
	{	"noconv",	BOOLEAN,	TRUE,	(void*)&fNoConv,
		"項目名を大文字に加工しない"					},
	{	"nofiller",	BOOLEAN,	TRUE,	(void*)&fNoFiller,
		"レコード長調整のFILLERを入れない"				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"レコードのあるディレクトリ"					},
	{	"ddir",		STRING,		TRUE,	(void*)&D_Dir,
		"定義格納ディレクトリ"	 						},
	{	"ld",		STRING,		TRUE,	(void*)&LD_Name,
		"LD名"						 					},
	{	"bd",		STRING,		TRUE,	(void*)&BD_Name,
		"BD名"						 					},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fNoConv = FALSE;
	fNoFiller = FALSE;
	fFiller = FALSE;
	fSPA = FALSE;
	fLinkage = FALSE;
	fScreen = FALSE;
	fDB = FALSE;
	fDBREC = FALSE;
	fDBCOMM = FALSE;
	fDBPATH = FALSE;
	fMCP = FALSE;
	fFull = FALSE;
	ArraySize = SIZE_DEFAULT_ARRAY_SIZE;
	TextSize = SIZE_DEFAULT_TEXT_SIZE;
	Prefix = "";
	RecName = "";
	LD_Name = NULL;
	BD_Name = NULL;
	fLDW = FALSE;
	fLDR = FALSE;
	fWindowPrefix = FALSE;
	fRecordPrefix = FALSE;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	Lang = "OpenCOBOL";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*name
	,			*ext;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("copygen",NULL);
	ConvSetLanguage(Lang);
	Conv = NewConvOpt();
	ConvSetSize(Conv,TextSize,ArraySize);
	if		(  fl  !=  NULL  ) {
		name = fl->name;
		ext = GetExt(name);
		if		(  fDBREC  ) {
			MakeDBREC(name);
		} else
		if		(	(  !stricmp(ext,".rec")  )
				||	(  !stricmp(ext,".db")  ) ) {
			MakeFromRecord(name);
		}
	} else {
		if		(  fScreen  ) {
			if		(  LD_Name  ==  NULL  ) {
				PutLevel(1,TRUE);
				PutString("scrarea");
				printf(".\n");
				PutLevel(2,TRUE);
				PutString("screendata");
				printf(".\n");
			} else {
				MakeLD();
			}
		} else
		if		(  fLinkage  ) {
			MakeLinkage();
		} else
		if		(  fDB  ) {
			MakeDB();
		} else
		if		(  fDBCOMM  ) {
			MakeDBCOMM();
		} else
		if		(  fDBPATH  ) {
			MakeDBPATH();
		} else
		if		(  fMCP  ) {
			MakeMCP();
		} else
		if		(  fLDW  ) {
			fMCP = TRUE;
			fSPA = TRUE;
			fLinkage = TRUE;
			fScreen = TRUE;
			RecName = "ldw";
			Prefix = "ldw-";
			MakeLD();
		} else
		if		(  fLDR  ) {
			fMCP = TRUE;
			fSPA = TRUE;
			fLinkage = TRUE;
			fScreen = TRUE;
			RecName = "ldr";
			Prefix = "ldr-";
			MakeLD();
		}
	}
	return	(0);
}
