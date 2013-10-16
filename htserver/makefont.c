/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2006-2008 Ogochan & JFBA (Japan Federation of Bar Association)
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

#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	"const.h"
#ifdef	HAVE_GD
#include	"libmondai.h"
#include	"gd.h"
#include	"option.h"
#include	"debug.h"

#define	FONT_BASE	"/usr/share/fonts/truetype/kochi/kochi-gothic-subst.ttf"
#define	FONT_EXT	"./nichiben.ttf"

static	char	*FontTemplate;
static	char	*FontBase;
static	char	*FontExt;
static	char	*FG;
static	char	*BG;
static	int		Top;

static	ARG_TABLE	option[] = {
	{	"top",		INTEGER,	TRUE,	(void*)&Top,
		"画像の高さ調整"								},
	{	"base",		STRING,		TRUE,	(void*)&FontBase,
		"標準文字のフォントファイル名"					},
	{	"ext",		STRING,		TRUE,	(void*)&FontExt,
		"外字文字のフォントファイル名"					},
	{	"font",		STRING,		TRUE,	(void*)&FontTemplate,
		"フォント代替イメージファイル名生成テンプレート"},
	{	"fgcolor",	STRING,		TRUE,	(void*)&FG,
		"描画色"										},
	{	"bgcolor",	STRING,		TRUE,	(void*)&BG,
		"背景色"										},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	FontBase = FONT_BASE;
	FontExt = FONT_EXT;
	FontTemplate = "%d/%04X.png";
	FG = "000000";
	BG = "FFFFFF";
	Top = 0;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	gdImagePtr	im;
	int		size
		,	x
		,	y;
	int		code1
		,	code2
		,	code;
	int		r
		,	g
		,	b
		,	bg
		,	fg
		,	tr
		,	brect[8];
	FILE	*fp;
	char	*font;
	char	str[SIZE_LONGNAME+1]
		,	file[SIZE_LONGNAME+1];
	FILE_LIST	*fl
		,		*p;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	argc = 0;
	for	( p = fl ; p != NULL ; p = p->next ) {
		argc ++;
	}
	InitMessage("makefont",NULL);
	switch	(argc) {
	  case	3:
		size = atoi(fl->name);
		fl = fl->next;
		code1 = (int)HexToInt(fl->name,strlen(fl->name));
		fl = fl->next;
		code2 = (int)HexToInt(fl->name,strlen(fl->name));
		break;
	  case	2:
		size = 10;
		code1 = (int)HexToInt(fl->name,strlen(fl->name));
		fl = fl->next;
		code2 = (int)HexToInt(fl->name,strlen(fl->name));
		break;
	  default:
		printf("%s [h] code1 code2\n",argv[0]);fflush(stdout);
		exit(1);
	}

	for	( code = code1 ; code  <= code2 ; code ++ ) {
		if		(  code  <  0xE000  ) {
			font = FontBase;
		} else {
			font = FontExt;
		}
		sprintf(str,"&#%d;",code);
		gdImageStringTTF(NULL,brect,0,font,size,0.0,0,0,str);
		x = brect[2] - brect[6];
		y = brect[3] - brect[7] + Top;
		im = gdImageCreate(x,y);

		r = (int)HexToInt(&BG[0],2);
		g = (int)HexToInt(&BG[2],2);
		b = (int)HexToInt(&BG[4],2);
		bg = gdImageColorAllocate(im,r,g,b);
		if		(  ( tr = gdImageColorExact(im,255,255,255) )  !=  -1  ) {
			gdImageColorTransparent(im,tr);
		}

		r = (int)HexToInt(&FG[0],2);
		g = (int)HexToInt(&FG[2],2);
		b = (int)HexToInt(&FG[4],2);
		fg = gdImageColorAllocate(im,r,g,b);
		x = - brect[6];
		y = Top - brect[7];
		gdImageFilledRectangle(im,0,0,x,y,bg);
		gdImageStringTTF(im,brect,fg,font,size,0.0,x,y,str);
		sprintf(file,"%d/%04X.png",size,code);
		if		(  ( fp = Fopen(file,"w") )  !=  NULL  ) {
			gdImagePng(im,fp);
			fclose(fp);
		}
		gdImageDestroy(im);
	}
	return	(0);
}

#else
extern	int
main(
	int		argc,
	char	**argv)
{
	printf("not support\n");
	exit(1);
	return	(1);
}
#endif
