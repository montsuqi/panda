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
#define	MAIN
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define	_STYLE_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	<sys/stat.h>
#include	<gtk/gtk.h>
#include	"types.h"
#include	"libmondai.h"
#include	"styleLex.h"
#include	"styleParser.h"
#include	"debug.h"

static	char	*StyleFileName;
static	Bool	fError;
static	GHashTable	*Styles;

#define	GetSymbol	(StyleToken = StyleLex(FALSE))
#define	GetName		(StyleToken = StyleLex(TRUE))

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}
#undef	Error
#define	Error(msg)	{fError=TRUE;_Error((msg),StyleFileName,StylecLine);}

static	int
HexStringToInt(
	char	*p,
	size_t	len)
{
	int		ret;

	ret = 0;
	while	(  len  >  0  ) {
		ret *= 16;
		if		(	(  *p  >=  '0'  )
				&&	(  *p  <=  '9'  ) ) {
			ret += *p - '0';
		} else
		if		(	(  *p  >=  'a'  )
				&&	(  *p  <=  'f'  ) ) {
			ret += *p - 'a' + 10;
		} else
		if		(	(  *p  >=  'A'  )
				&&	(  *p  <=  'F'  ) ) {
			ret += *p - 'A' + 10;
		}
		p ++;
		len --;
	}
	return	(ret);
}

static	void
ParColor(
	GdkColor	*color)
{
	size_t	len;
	char	*p;

	if		(  GetSymbol  ==  '{'  ) {
		if		(  GetSymbol  ==  T_ICONST  ) {
			color->red = StyleComInt;
		} else {
			Error("red invalid");
		}
		if		(  GetSymbol  !=  ','  ) {
			Error(", missing");
		}
		if		(  GetSymbol  ==  T_ICONST  ) {
			color->green = StyleComInt;
		} else {
			Error("red invalid");
		}
		if		(  GetSymbol  !=  ','  ) {
			Error(", missing");
		}
		if		(  GetSymbol  ==  T_ICONST  ) {
			color->blue = StyleComInt;
		} else {
			Error("red invalid");
		}
		if		(  GetSymbol  !=  '}'  ) {
			Error("} not found");
		}
	} else
	if		(  StyleToken  ==  T_SCONST  ) {
		p = StyleComSymbol;
		len = strlen(StyleComSymbol) / 3;
		color->red = HexStringToInt(p,len);
		p += len;
		color->green = HexStringToInt(p,len);
		p += len;
		color->blue = HexStringToInt(p,len);
		switch	(len) {
		  case	1:
			color->red *= 4369;
			color->green *= 4369;
			color->blue *= 4369;
			break;
		  case	2:
			color->red *= 257;
			color->green *= 257;
			color->blue *= 257;
			break;
		  case	3:
			color->red *= 16;
			color->green *= 16;
			color->blue *= 16;
			break;
		}
	}
}

static	void
ParStyle(void)
{
	int			token;
	GtkStyle	*st
	,			*dst;
	int			state;	

dbgmsg(">ParStyle");
	dst = gtk_widget_get_default_style();
	GetSymbol;
	do {
		switch	(StyleToken) {
		  case	T_STYLE:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ( st = g_hash_table_lookup(Styles,StyleComSymbol) )
						   ==  NULL  ) {
					st = gtk_style_copy(dst);
					g_hash_table_insert(Styles,StrDup(StyleComSymbol),st);
				}
			} else {
				Error("style name not found");
				st = gtk_style_copy(dst);	/*	dummy	*/
			}
			if		(  GetSymbol  !=  '{'  ) {
				Error("{ missing");
			}
			GetSymbol;
			do {
				switch	(StyleToken) {
				  case	T_FONTSET:
					if		(  GetSymbol  !=  '='  ) {
						Error("= not found");
					}
					if		(  GetSymbol  ==  T_SCONST  ) {
						st->font = gdk_fontset_load(StyleComSymbol);
					} else {
						Error("font set not found");
					}
					break;
				  case	T_FG:
				  case	T_BG:
				  case	T_BG_PIXMAP:
				  case	T_TEXT:
				  case	T_BASE:
				  case	T_LIGHT:
				  case	T_DARK:
				  case	T_MID:
					token = StyleToken;
					state = 0;
					if		(  GetSymbol  ==  '['  ) {
						switch	(GetSymbol) {
						  case	T_NORMAL:
							state = GTK_STATE_NORMAL;
							break;
						  case	T_ACTIVE:
							state = GTK_STATE_ACTIVE;
							break;
						  case	T_INSENSITIVE:
							state = GTK_STATE_INSENSITIVE;
							break;
						  case	T_PRELIGHT:
							state = GTK_STATE_PRELIGHT;
							break;
						  case	T_SELECTED:
							state = GTK_STATE_SELECTED;
							break;
						  default:
							Error("state missing");
							break;
						}
						if		(  GetSymbol  !=  ']'  ) {
							Error("] missing");
						}
					} else {
						Error("[ missing");
					}
					if		(  GetSymbol  !=  '='  ) {
						Error("= not found");
					}
					switch	(token) {
					  case	T_FG:
						ParColor(&st->fg[state]);
						break;
					  case	T_BG:
						ParColor(&st->bg[state]);
						break;
					  case	T_TEXT:
						ParColor(&st->text[state]);
						break;
					  case	T_BASE:
						ParColor(&st->base[state]);
						break;
					  case	T_LIGHT:
						ParColor(&st->light[state]);
						break;
					  case	T_DARK:
						ParColor(&st->dark[state]);
						break;
					  case	T_MID:
						ParColor(&st->mid[state]);
						break;
					  case	T_BG_PIXMAP:
						Error("not support");
						break;
					}
					break;
				}
				GetSymbol;
			}	while	(  StyleToken  !=  '}'  );
			break;
		  default:
			Error("style missing");
			printf("token = %d(%c)\n",StyleToken,StyleToken);
			break;
		}
		GetSymbol;
	}	while	(  StyleToken  !=  T_EOF  );
dbgmsg("<ParStyle");
}

extern	void
StyleParserInit(void)
{
	StyleLexInit();
	Styles = NewNameHash();	 
}

void
ghfunc_styles_free (char *name, GtkStyle *value, gpointer user_data)
{
    xfree (name);
    gtk_style_unref (value);
    (void) user_data; /* escape warning */
}

void
StyleParserTerm(void)
{
    if (Styles != NULL) {
        g_hash_table_foreach(Styles, (GHFunc) ghfunc_styles_free, NULL);
        g_hash_table_destroy (Styles);
        Styles = NULL;
    }
}

extern	void
StyleParser(
	char	*name)
{
	FILE	*fp;
	struct	stat	stbuf;

dbgmsg(">StyleParser");
	if		(  stat(name,&stbuf)  ==  0  ) { 
		fError = FALSE;
		StylecLine = 1;
		if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
			StyleFileName = name;
			StyleFile = fp;
			ParStyle();
			fclose(fp);
		}
	}
dbgmsg("<StyleParser");
}

extern	GtkStyle	*
GetStyle(
	char	*name)
{
	GtkStyle	*p;

	if		(  ( p = g_hash_table_lookup(Styles,name) )  ==  NULL  ) {
		p = gtk_widget_get_default_style();
	}
	return	(p);
}

#ifdef	MAIN
static	char	*state[] = {
	"NORMAL", "ACTIVE", "PRELIGHT", "SELECTED", "INSENSITIVE" };

static	void
PrintStyle(
	GtkStyle	*value)
{
	int		i;

	printf("fontset = [%X]\n",(int)value->font);
	for	( i = 0 ; i < 5 ; i ++ ) {
		printf("fg[%s] = { %d, %d, %d }\n",state[i],
			   value->fg[i].red,value->fg[i].green,value->fg[i].blue);
		printf("bg[%s] = { %d, %d, %d }\n",state[i],
			   value->bg[i].red,value->bg[i].green,value->bg[i].blue);
	}
}
static	void
Dump(
	char		*name,
	GtkStyle	*value,
	gpointer	user_data)
{
	printf("name = [%s]\n",name);
	PrintStyle(value);
}
	
extern	int
main(
	int		argc,
	char	**argv)
{
	gtk_set_locale();
	gtk_init(&argc, &argv);
	
	StyleParserInit();
	StyleParser("testrc");

	printf("*** dump ***\n");
	g_hash_table_foreach(Styles,(GHFunc)Dump,NULL);
	printf("*** dump ***\n");
	PrintStyle(GetStyle("green"));

	return	(0);
}
#endif
