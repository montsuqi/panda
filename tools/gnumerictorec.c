/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2006-2008 Ogochan.
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
static	int		Level;

#define	GNUMERIC_NS			"http://www.gnumeric.org/v10.dtd"
#define	DEFAULT_LEVEL		5

typedef	struct {
	char	*text;
	int		id;
}	CellAttribute;

typedef	struct {
	int		cId;
	int		maxrow;
	int		maxcol;
	CellAttribute	**cell;
	int		*horizontal;
	int		*virtical;
}	Table;

extern	Table	*
NewTable(
	int		maxrow,
	int		maxcol)
{
	Table	*table;
	CellAttribute	*cell;
	size_t	size;
	int			i;

	table = New(Table);
	size = ( maxrow + 1 ) * ( maxcol + 1 );
	table->maxcol = maxcol;
	table->maxrow = maxrow;
	table->cell = (CellAttribute **)xmalloc(sizeof(CellAttribute *) * size);
	for	( i = 0 ; i < size ; i ++ ) {
		cell = New(CellAttribute);
		cell->text = NULL;
		cell->id = -1;
		table->cell[i] = cell;
	}
	size = ( maxrow + 2 ) * ( maxcol + 2 );
	table->horizontal = (int *)xmalloc(sizeof(int) * size);
	table->virtical = (int *)xmalloc(sizeof(int) * size);
	for	( i = 0 ; i < size ; i ++ ) {
		table->horizontal[i] = 0;
		table->virtical[i] = 0;
	}
	table->cId = 0;
	return	(table);
}

extern	void
FreeTable(
	Table	*table)
{
	int		size
		,	i;

	size = ( table->maxrow + 1 ) * ( table->maxcol + 1 );
	for	( i = 0 ; i < size ; i ++ ) {
		if		(  table->cell[i]->text  !=  NULL  ) {
			xfree(table->cell[i]->text);
		}
		xfree(table->cell[i]);
	}
	xfree(table->cell);
	xfree(table->horizontal);
	xfree(table->virtical);
	xfree(table);
}

extern	CellAttribute	*
GetCell(
	Table	*table,
	int		row,
	int		col)
{
	CellAttribute	*ret;

	if		(	(  row  >=  0  )
			&&	(  col  >=  0  )
			&&	(  row  <=  table->maxrow  )
			&&	(  col  <=  table->maxcol  ) ) {
		ret = table->cell[(table->maxcol + 1) * row + col];
	} else {
		ret = NULL;
	}
	return	(ret);
}

static	void
ReadCells(
	xmlNodePtr	Sheet,
	Table		*table)
{
	xmlNodePtr	Cells
		,		Cell
		,		Text;
	int			row
		,		col;
	char		*text;
	CellAttribute	*cell;

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
			if		(  ( cell = GetCell(table,row,col) )  !=  NULL  ) {
				if		(  text  !=  NULL  ) {
					cell->text = StrDup(text);
				} else {
					cell->text = NULL;
				}
				cell->id = table->cId ++;
			}
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
	Table		*table)
{
	xmlNodePtr	MergedRegions
		,		Merge
		,		Text;
	CellAttribute	*cell;
	int			row
		,		col
		,		x1
		,		y1
		,		x2
		,		y2;
	char		buff[16]	/*	XXnnnnn:XXnnnnn		*/
		,		*p;

ENTER_FUNC;
	if		(  ( MergedRegions = SearchNode(Sheet,GNUMERIC_NS,"MergedRegions",NULL,NULL) )
			   !=  NULL  ) {
		Merge = XMLNodeChildren(MergedRegions);
		while	(  Merge  !=  NULL  ) {
			if		(  ( Text = XMLNodeChildren(Merge) )  !=  NULL  ) {
				strcpy(buff,XMLNodeContent(Text));
				p = strchr(buff,':');
				*p = 0;
				ParseCellName(buff,&x1,&y1);
				ParseCellName(p + 1,&x2,&y2);
				for	( row = y1 ; row <= y2 ; row ++ ) {
					for	( col = x1 ; col <= x2 ; col ++ ) {
						if		(  ( cell = GetCell(table,row,col) )  !=  NULL  ) {
							cell->id = table->cId;
						}
					}
				}
				table->cId ++;
			}
			Merge = XMLNodeNext(Merge);
		}
	}
LEAVE_FUNC;
}

#define	RULE_INDEX(table,row,col)	(((table)->maxcol + 2) * (row) + (col))
static	void
SetRule(
	Table	*table,
	int		row,
	int		col,
	int		top,
	int		bottom,
	int		left,
	int		right)
{
	if		(	(  row  >=  0  )
			&&	(  col  >=  0  )
			&&	(  row  <=  table->maxrow  )
			&&	(  col  <=  table->maxcol  ) ) {
		if		(  top  >  0  ) {
			table->virtical[RULE_INDEX(table,row,col)] = top;
		}
		if		(  bottom  >  0  ) {
			table->virtical[RULE_INDEX(table,( row + 1 ),col)] = bottom;
		}
		if		(  left  >  0  ) {
			table->horizontal[RULE_INDEX(table,row,col)] = left;
		}
		if		(  right  >  0  ) {
			table->horizontal[RULE_INDEX(table,row,( col + 1 ))] = right;
		}
	}
}

static	void
ReadStyles(
	xmlNodePtr	Sheet,
	Table		*table)
{
	xmlNodePtr	StyleRegion
		,		Styles
		,		Style
		,		StyleBorder
		,		pos;
	int			row
		,		col
		,		x1
		,		y1
		,		x2
		,		y2;
	int			top
		,		bottom
		,		left
		,		right;
	CellAttribute	*cell1
		,			*cell2;

ENTER_FUNC;
	Styles = SearchNode(Sheet,GNUMERIC_NS,"Styles",NULL,NULL);
	StyleRegion = XMLNodeChildren(Styles);
	while	(  StyleRegion  !=  NULL  ) {
		x1 = atoi(XMLGetProp(StyleRegion,"startCol"));
		y1 = atoi(XMLGetProp(StyleRegion,"startRow"));
		x2 = atoi(XMLGetProp(StyleRegion,"endCol"));
		y2 = atoi(XMLGetProp(StyleRegion,"endRow"));
		if		(  ( Style = XMLNodeChildren(StyleRegion) )  !=  NULL  ) {
			StyleBorder = SearchNode(Style,GNUMERIC_NS,"StyleBorder",NULL,NULL);
			pos = XMLNodeChildren(StyleBorder);
			top = 0;
			bottom = 0;
			left = 0;
			right = 0;
			while	(  pos  !=  NULL  ) {
				if		(  strcmp(XMLName(pos),"Top")  ==  0  ) {
					top = atoi(XMLGetProp(pos,"Style"));
				}
				if		(  strcmp(XMLName(pos),"Bottom")  ==  0  ) {
					bottom = atoi(XMLGetProp(pos,"Style"));
				}
				if		(  strcmp(XMLName(pos),"Left")  ==  0  ) {
					left = atoi(XMLGetProp(pos,"Style"));
				}
				if		(  strcmp(XMLName(pos),"Right")  ==  0  ) {
					right = atoi(XMLGetProp(pos,"Style"));
				}
				pos = XMLNodeNext(pos);
			}
			for	( row = y1 ; row <= y2 ; row ++ ) {
				for	(  col = x1 ; col <= x2 ; col ++ ) {
					SetRule(table,row,col,top,bottom,left,right);
				}
			}
		}
		StyleRegion = XMLNodeNext(StyleRegion);
	}
	/*	clean up rule	*/
	for	( row = 0 ; row <= table->maxrow + 1; row ++ ) {
		for	( col = 0 ; col <= table->maxcol + 1 ; col ++ ) {
			if		(  table->virtical[RULE_INDEX(table,row,col)]  >  0  ) {
				if		(	(  row  ==  0  )
						||	(  ( cell1 = GetCell(table,row-1,col) )  ==  NULL  )
						||	(  ( cell2 = GetCell(table,row  ,col) )  ==  NULL  )
						||	(  cell1->id  !=  cell2->id  )
						||	(  cell1->id  ==  -1         )
						||	(  cell2->id  ==  -1         ) ) {
					/*	nop	*/
				} else {
					table->virtical[RULE_INDEX(table,row,col)] = 0;
				}
			}
		}
		for	( col = 0 ; col <= table->maxcol + 1 ; col ++ ) {
			if		(  table->horizontal[RULE_INDEX(table,row,col)]  >  0  ) {
				if		(	(  col  ==  0  )
						||	(  ( cell1 = GetCell(table,row,col-1) )  ==  NULL  )
						||	(  ( cell2 = GetCell(table,row,col  ) )  ==  NULL  )
						||	(  cell1->id  !=  cell2->id  )
						||	(  cell1->id  ==  -1         )
						||	(  cell2->id  ==  -1         ) ) {
					/*	nop	*/
				} else {
					table->horizontal[RULE_INDEX(table,row,col)] = 0;
				}
			}
		}
	}
LEAVE_FUNC;
}

#ifdef	DEBUG
static	void
DumpTable(
	Table	*table)
{
	int		row
		,	col;
	CellAttribute	*cell;

	for	( row = 0 ; row <= table->maxrow + 1; row ++ ) {
		printf("%4d:",row);
		for	( col = 0 ; col <= table->maxcol + 1 ; col ++ ) {
			if		(  table->virtical[RULE_INDEX(table,row,col)]  >  0  ) {
				printf(" ----");
			} else {
				printf("     ");
			}
		}
		printf("\n");
		printf("%4d:",row);
		for	( col = 0 ; col <= table->maxcol + 1 ; col ++ ) {
			if		(  table->horizontal[RULE_INDEX(table,row,col)]  >  0  ) {
				printf("|");
			} else {
				printf(" ");
			}
			if		(  ( cell = GetCell(table,row,col) )  !=  NULL  ) {
				printf("%4d",cell->id);
			}
		}
		printf("\n");
	}
}
#endif

extern	Table	*
LoadGnumeric(
	char	*fname)
{
	xmlDocPtr	doc;
	xmlNodePtr	Sheet;
	Table		*table;
	int			maxcol
		,		maxrow;

ENTER_FUNC;
	doc = xmlReadFile(fname,NULL,XML_PARSE_NOBLANKS);
	Sheet = SearchNode(xmlDocGetRootElement(doc),GNUMERIC_NS,"Sheet",NULL,NULL);
	maxrow = atoi(XMLGetPureText(SearchNode(Sheet,GNUMERIC_NS,"MaxRow",NULL,NULL)));
	maxcol = atoi(XMLGetPureText(SearchNode(Sheet,GNUMERIC_NS,"MaxCol",NULL,NULL)));
	table = NewTable(maxrow,maxcol);
	ReadCells(Sheet,table);
	ReadRegions(Sheet,table);
	ReadStyles(Sheet,table);
#ifdef	DEBUG
	DumpTable(table);
#endif
	xmlFreeDoc(doc);
LEAVE_FUNC;
	return	(table);
}

static	void
PutTab(
	int	n)
{
	for	( ; n >= 0 ; n -- ) {
		printf("\t");
	}
}

static	void
WriteDefines(
	Table	*table)
{
	int		last
		,	i
		,	j
		,	dim[Level];
	CellAttribute	*cell;
	char		buff[SIZE_BUFF];
	char	*p;
	
	last = 0;
	for	( i = 2 ; i <= table->maxrow ; i ++ ) {
		if		(  GetCell(table,i,0)->id  >=  0  ) {
			for	( j = 0 ; j < Level ; j ++ ) {
				cell = GetCell(table,i,j);
				if		(  cell->text  !=  NULL  ) {
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
					strcpy(buff,cell->text);
					if		(  ( p = strchr(buff,'[') )  !=  NULL  ) {
						*p = 0;
						dim[j] = atoi(p+1);
					} else {
						dim[j] = 0;
					}
					printf("%s",buff);
					if		(	(  j  ==  Level - 1  )
							||	(  GetCell(table,i,j+1)->text  ==  NULL  ) ) {
						printf("\t");
						last = j;
						break;
					} else {
						printf("\t{\n");
					}
				}
			}
			if		(  GetCell(table,i,Level)->text  !=  NULL  ) {
				if		(  dim[last]  >  0  ) {
					printf("%s[%d];",GetCell(table,i,Level)->text,dim[last]);
				} else {
					printf("%s;",GetCell(table,i,Level)->text);
				}
				if		(	(  !fComment  )
						||	(  GetCell(table,i,Level+1)->text  ==  NULL  ) ) {
					printf("\n");
				} else {
					printf("\t#\t%s\n",GetCell(table,i,Level+1)->text);
				}
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
}

static	void
ProcessOne(
	char	*fname)
{
	char		*rname;
	char		*p;

ENTER_FUNC;
	rname = StrDup(basename(fname));
	if		(  ( p = strstr(rname,".gnumeric") )  !=  NULL  ) {
		*p = 0;
	}
	printf("%s\t{\n",rname);
	WriteDefines(LoadGnumeric(fname));
	printf("};\n");
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"comment",	BOOLEAN,	TRUE,	(void*)&fComment,
		"項目の説明を入れる"							},
	{	"level",	INTEGER,	TRUE,	(void*)&Level,
		"階層の数" 										},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fComment = FALSE;
	Level = DEFAULT_LEVEL;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("checkdir",NULL);

	ProcessOne(fl->name);

	return	(0);
}

