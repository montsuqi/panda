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
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<glib.h>
#include	<libgen.h>
#include	<string.h>
#include	<sys/stat.h>

#include	<libxslt/xslt.h>
#include	<libxslt/xsltInternals.h>
#include	<libxslt/transform.h>
#include	<libxslt/xsltutils.h>
#include 	<libexslt/exslt.h>
#include	<libexslt/exsltconfig.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"xxml.h"
#include	"load.h"
#include	"option.h"
#include	"debug.h"

static	Bool	fComment;

#define	GNUMERIC_NS			"http://www.gnumeric.org/v10.dtd"

typedef	struct {
	char	*text;
	int		id;
}	CellAttribute;

static	int		_MaxRow;
static	int		_MaxCol;
#define	INDEX(i,j)	((i)*_MaxCol+(j))

static	void
ReadCells(
	xmlNodePtr	Sheet,
	CellAttribute	**table)
{
	xmlNodePtr	Cells
		,		Cell
		,		Text;
	int			row
		,		col;
	char		*text;

ENTER_FUNC;
	Cells = SearchNode(Sheet,GNUMERIC_NS,"Cells",NULL,NULL);
	Cell = XMLNodeChildren(Cells);
	while	(  Cell  !=  NULL  ) {
		if		(	(  strcmp(XMLNsBody(Cell),GNUMERIC_NS)  ==  0  )
				&&	(  strcmp(XMLName(Cell),"Cell")         ==  0  ) ) {
			row = atoi(XMLGetProp(Cell,"Row"));
			col = atoi(XMLGetProp(Cell,"Col"));
			if		(  ( Text = XMLNodeChildren(Cell) )  !=  NULL  ) {
				text = XMLNodeContent(Text);
			} else {
				text = NULL;
			}
			table[INDEX(row,col)]->text = text;
			table[INDEX(row,col)]->id = 0;
		} else {
			fprintf(stderr,"<%s:%s>\n",XMLNsBody(Cell),XMLName(Cell));
		}
		Cell = XMLNodeNext(Cell);
	}
LEAVE_FUNC;
}

static	void
ParseCellName(
	char	*name,
	int		*x,
	int		*y)
{
	char	*p;
	int		base;

	p = name;
	base = 1;
	*x = 0;
	while	(	(  *p  !=  0    )
			&&	(  isalpha(*p)  ) ) {
		(*x) *= base;
		(*x) += *p - 'A';
		base *= 26;
		p ++;
	}
	*y = 0;
	base = 1;
	while	(  *p  !=  0  ) {
		(*y) *= base;
		(*y) += *p - '0';
		base *= 10;
		p ++;
	}
	*y -= 1;	/*	1 origine	*/
}

static	void
ReadRegions(
	xmlNodePtr	Sheet,
	CellAttribute	**table)
{
	xmlNodePtr	MergedRegions
		,		Merge
		,		Text;
	int			id;
	int			row
		,		col
		,		x1
		,		y1
		,		x2
		,		y2;
	char		buff[2+5+1+2+5+1]
		,		*p;

ENTER_FUNC;
	MergedRegions = SearchNode(Sheet,GNUMERIC_NS,"MergedRegions",NULL,NULL);
	Merge = XMLNodeChildren(MergedRegions);
	id = 1;
	while	(  Merge  !=  NULL  ) {
		if		(  ( Text = XMLNodeChildren(Merge) )  !=  NULL  ) {
			strcpy(buff,XMLNodeContent(Text));
			p = strchr(buff,':');
			*p = 0;
			ParseCellName(buff,&x1,&y1);
			ParseCellName(p + 1,&x2,&y2);
			for	( row = y1 ; row <= y2 ; row ++ ) {
				for	( col = x1 ; col <= x2 ; col ++ ) {
					table[INDEX(row,col)]->id = id;
				}
			}
			id ++;
		}
		Merge = XMLNodeNext(Merge);
	}
LEAVE_FUNC;
}

static	void
PutTab(
	int	n)
{
	for	( ; n >= 0 ; n -- ) {
		printf("\t");
	}
}

#define	LEVEL		5

static	void
LoadGnumeric(
	char	*fname)
{
	xmlDocPtr	doc;
	xmlNodePtr	Sheet;
	char		*rname;
	char		*p;
	int			i
		,		j
		,		last
		,		dim[LEVEL];
	char		buff[SIZE_BUFF];
	CellAttribute	**table;

ENTER_FUNC;
	doc = xmlReadFile(fname,NULL,XML_PARSE_NOBLANKS);
	rname = StrDup(basename(fname));
	if		(  ( p = strstr(rname,".gnumeric") )  !=  NULL  ) {
		*p = 0;
	}
	printf("%s\t{\n",rname);

	Sheet = SearchNode(xmlDocGetRootElement(doc),GNUMERIC_NS,"Sheet",NULL,NULL);
	_MaxRow = atoi(XMLGetPureText(SearchNode(Sheet,GNUMERIC_NS,"MaxRow",NULL,NULL))) + 1;
	_MaxCol = atoi(XMLGetPureText(SearchNode(Sheet,GNUMERIC_NS,"MaxCol",NULL,NULL))) + 1;

	table = (CellAttribute **)xmalloc(sizeof(CellAttribute *) * _MaxRow * _MaxCol);
	for	( i = 0 ; i < _MaxRow ; i ++ ) {
		for	( j = 0 ; j < _MaxCol ; j ++ ) {
			table[INDEX(i,j)] = New(CellAttribute);
			table[INDEX(i,j)]->text = NULL;
			table[INDEX(i,j)]->id = -1;
		}
	}

	ReadCells(Sheet,table);
	ReadRegions(Sheet,table);
#ifdef	DEBUG
	for	( i = 0 ; i < _MaxRow ; i ++ ) {
		printf("%4d:",i+1);
		for	( j = 0 ; j < _MaxCol ; j ++ ) {
			printf("%2d  ",table[INDEX(i,j)]->id);
		}
		printf("\n");
	}
#endif
	last = 0;
	for	( i = 2 ; i < _MaxRow ; i ++ ) {
		if		(  table[INDEX(i,0)]->id  >=  0  ) {
			for	( j = 0 ; j < LEVEL ; j ++ ) {
				if		(  table[INDEX(i,j)]->text  !=  NULL  ) {
					while	(  j  <  last  ) {
						last --;
						PutTab(last);
						if		(  dim[last]  >  0  ) {
							printf("}[%d];\n",dim[last]);
						} else {
							printf("};\n");
						}
					}
					PutTab(j);
					strcpy(buff,table[INDEX(i,j)]->text);
					if		(  ( p = strchr(buff,'[') )  !=  NULL  ) {
						*p = 0;
						dim[j] = atoi(p+1);
					} else {
						dim[j] = 0;
					}
					printf("%s",buff);
					if		(	(  j  ==  LEVEL - 1  )
							||	(  table[INDEX(i,j+1)]->text  ==  NULL  ) ) {
						printf("\t");
						last = j;
						break;
					} else {
						printf("\t{\n");
					}
				}
			}
			printf("%s;",table[INDEX(i,LEVEL)]->text);
			if		(	(  !fComment  )
					||	(  table[INDEX(i,LEVEL+1)]->text  ==  NULL  ) ) {
				printf("\n");
			} else {
				printf("\t#\t%s\n",table[INDEX(i,LEVEL+1)]->text);
			}
		}
	}
	while	(  last  >  0  ) {
		last --;
		PutTab(last);
		if		(  dim[last]  >  0  ) {
			printf("}[%d];\n",dim[last]);
		} else {
			printf("};\n");
		}
	}
	printf("};\n");
LEAVE_FUNC;
	xmlFreeDoc(doc);
}

static	ARG_TABLE	option[] = {
	{	"comment",	BOOLEAN,	TRUE,	(void*)&fComment,
		"項目の説明を入れる"							},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fComment = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("checkdir",NULL);

	LoadGnumeric(fl->name);

	return	(0);
}

