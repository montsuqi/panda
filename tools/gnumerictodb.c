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

#define	GNUMERIC_NS			"http://www.gnumeric.org/v10.dtd"

static	void
LoadGnumeric(
	char	*fname)
{
	xmlDocPtr	doc;
	xmlNodePtr	Sheet
		,		Cells
		,		Cell
		,		Text;
	int			MaxRow
		,		Row
		,		Col
		,		r;
	char		*text
		,		*jname
		,		*name
		,		*type;
	char		**primary
		,		**pr;
	char		*rname;
	char		*p;

	doc = xmlReadFile(fname,NULL,XML_PARSE_NOBLANKS);
	Sheet = SearchNode(xmlDocGetRootElement(doc),GNUMERIC_NS,"Sheet",NULL,NULL);
	MaxRow = atoi(XMLGetPureText(SearchNode(Sheet,GNUMERIC_NS,"MaxRow",NULL,NULL))) + 1;
	primary = (char **)xmalloc(MaxRow * sizeof(char *));
	for	( r = 0 ; r < MaxRow ; r ++ ) {
		primary[r] = NULL;
	}
	pr = primary;
	Cells = SearchNode(Sheet,GNUMERIC_NS,"Cells",NULL,NULL);
	Cell = XMLNodeChildren(Cells);
	rname = StrDup(basename(fname));
	if		(  ( p = strstr(rname,".gnumeric") )  !=  NULL  ) {
		*p = 0;
	}
	printf("%s\t{\n",rname);
	for	( r = 1 ;(	(  r     <   MaxRow  )
				&&	(  Cell  !=  NULL    ) ); ) {
		if		(	(  strcmp(XMLNsBody(Cell),GNUMERIC_NS)  ==  0  )
				&&	(  strcmp(XMLName(Cell),"Cell")         ==  0  ) ) {
			Row = atoi(XMLGetProp(Cell,"Row"));
			Col = atoi(XMLGetProp(Cell,"Col"));
			if		(  ( Text = XMLNodeChildren(Cell) )  !=  NULL  ) {
				text = XMLNodeContent(Text);
			} else {
				text = NULL;
			}
			if		(  Row  >  0  ) {
				switch	(Col) {
				  case	0:
					jname = text;
					break;
				  case	1:
					name = text;
					break;
				  case	2:
					type = text;
					printf("\t%s\t%s;\t\t#\t%s\n",name,type,jname);
					break;
				  case	3:
					if		(	(  text            !=  NULL  )
							&&	(  toupper(*text)  ==  'P'   ) ) {
						*pr = name;
						pr ++;
					}
					break;
				  default:
					break;
				}
			}
			if		(  Row  >  r  ) {
				r ++;
			}
		} else {
			fprintf(stderr,"<%s:%s>\n",XMLNsBody(Cell),XMLName(Cell));
		}
		Cell = XMLNodeNext(Cell);
	}
	printf("};\n");
	printf("primary\t{\n");
	for	( pr = primary ; *pr != NULL ; pr ++ ) {
		printf("\t%s;\n",*pr);
	}
	printf("};\n");
	printf("path\tprimary\t{\n");
	printf("\tDBSELECT\t{\n");
	printf("\t\tDECLARE\t%s_primary_csr\tCURSOR\tFOR\n",rname);
	printf("\t\tSELECT\t*\n");
	printf("\t\t\tWHERE\n");
	for	( pr = primary ; *pr != NULL ;  ) {
		printf("\t\t\t\t%s = :%s",*pr,*pr);
		pr ++;
		if		(  *pr  !=  NULL  ) {
			printf(",\n");
		} else {
			printf("\n");
		}
	}
	printf("\t\t;\n");
	printf("\t};\n");
	printf("};\n");

	xfree(rname);
	xmlFreeDoc(doc);
}

static	ARG_TABLE	option[] = {
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
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

