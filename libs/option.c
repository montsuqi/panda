/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1991-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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

/*	option parser	Copyright TSG & Ogochan
*
*	91.09.30.	Ogochan	load from dviprt
*	91.10.01.	Ogochan	modify to my style
*	91.12.04.	Ogochan	add file name list
*	91.12.05.	Ogochan	add print default value for help
*	92.06.18.	Ogochan	fix files list bug
*	93.02.15.	funtech	fix usage & no parameter
*	93.09.21.	ogochan	fix parameter list
*	98.08.29.	ogochan	name=param style
*	00.10.16.	ogochan	add GetExt
*	01.05.30.	ogochan	null print
*	03.06.02.	ogochan	add help flag("h", "H")
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"config.h"
#include	"types.h"
#include	"option.h"
#include	"gettext.h"

#define	MAX_LINE 8192

extern	char	*
GetExt(
	char	*name)
{
	char	*p;

	p = name + strlen(name);
	while	(	(  p   !=  name  )
			 &&	(  *p  !=  '\\'  )
			 &&	(  *p  !=  '/'   )
			 &&	(  *p  !=  '.'   ) )	{
		p --;
	}
	return	(p);
}

extern	void
ChangeExt(
	char	*dist,
	char	*org,
	char	*ext)
{	char	*p;

	strcpy(dist,org);
	p = GetExt(dist);
	if		(  *p   !=  '.'  ) {
		p = dist + strlen(dist);
	}
	strcpy(p,ext);
}

static	char	*
CheckParam(
	char	*line,
	char	*token)
{
	char	*ret
		,	*p;
	char	buf[MAX_LINE+1];

	strcpy(buf,line);
	p = buf;
	while	(	( *p  !=  0  )
			&&	(  !isspace(*p)  ) )	p ++;
	*p = 0;
	if		(  strcmp(buf,token)  ==  0  )	{
		ret = line + strlen(token);
		while	(  isspace(*ret)  )	ret ++;
		if		(  *ret  ==  '='  ) {
			ret ++;
		}
	} else {
		ret = NULL;
	}

	return(ret);
}

static	void
SetVar(
	ARG_TABLE	*option,
	char		*arg)
{	char	*endptr;

	while	(  isspace(*arg) )	{
		arg ++;
	}
	switch( option->type ) {
	  case BOOLEAN :
		if		(  *arg  ==  '+'  )	{
	      *(Bool *)option->var = FALSE; /* TRUE;*/
		} else {
			if		(  *arg  ==  '-'  )	{
				*(Bool *)option->var = TRUE; /* FALSE;*/
			} else {
				*(Bool *)option->var = 
				  ( *(Bool *)option->var == TRUE ) ? FALSE : TRUE;
			}
		}
        break;

	  case INTEGER :
		*(int * )option->var = (int)strtol(arg,&endptr,0);
		break;
	  case LONGINT :
		*(long *)option->var = strtol(arg,&endptr,0);
		break;
	  case PROCEDURE :
		(*(void (*)(char *))(option->var))( arg );
		break;
	  case STRING:
		if		(  strlen(arg)  !=  0  )	{
			*(char **)option->var = strdup(arg);
		} else {
			*(char **)option->var = NULL;
		}
		break;
	  default :
		break;
	}
}

static	void
PrintVar(
	ARG_TABLE	option)
{
	switch( option.type ) {
	  case BOOLEAN :
		if		(  *(Bool *)option.var  )	{
			printf("ON");
		} else {
			printf("OFF");
		}
        break;
	  case INTEGER :
		printf("%d",*(int * )option.var);
		break;
	  case LONGINT :
		printf("%ld",*(long *)option.var);
		break;
	  case PROCEDURE :
		break;
	  case STRING :
		if		(  *(char **)option.var  !=  NULL  ) {
			printf("%s",*(char **)option.var);
		}
		break;
	  default :
		break;
	}
}

static	ARG_TABLE	*LastArg;
static	Bool
AnalizeLine(
	ARG_TABLE	*tbl,
	char		*line)
{	Bool	rc;
	char	*arg;

	rc = FALSE;
	if		(  LastArg  ==  NULL  )	{
		if		(  *line  !=  ';'  )	{
			for ( ;	(	(  tbl->option  !=  NULL  )
					 &&	(  !rc                    ) ) ; tbl++ )	{
				if		(  ( arg = CheckParam(line,tbl->option) )  != NULL  ) {
					if		(  strlen(arg)  >  0  )	{
						SetVar( tbl, arg );
					} else {
						if		(  tbl->type  ==  BOOLEAN  )	{
							SetVar( tbl, arg );
						} else {
							LastArg = tbl;
						}
					}
					rc = TRUE;
				}
			}
		}
	} else {
		SetVar(LastArg,line);
		LastArg = NULL;
		rc = TRUE;
	}
    
	return	(rc);
}

extern	void
PrintUsage(
	ARG_TABLE	*tbl,
	char		*comment,
	char		*help)
{	int		i;

	bindtextdomain(PACKAGE, LOCALEDIR);

	printf("%s\n",comment);
	for ( i = 0 ; tbl[i].option != NULL ; i++ )	{
		if		(  tbl[i].message  !=  NULL  ) {
			printf( "  -%-12s : %-40s", tbl[i].option, _d(tbl[i].message) );
			if		(  tbl[i].defval  )	{
				printf("\t[");
				PrintVar(tbl[i]);
				printf("]");
			}
			printf("\n");
		}
	}
	if		(  help  !=  NULL  ) {
		printf("%s",_d(help));
	}
}

extern	FILE_LIST	*
GetOption(
	ARG_TABLE	*tbl,
	int			argc,
	char		**argv,
	char		*help)
{	int		c;
	char	*p
	,		*q
	,		*cmd;
	char	buff[MAX_LINE+1];
	FILE	*fpParam;
	FILE_LIST	*fl
	,			*next
	,			*next1;
	Bool	isParam;

	fl = NULL;
	LastArg = NULL;
	cmd = argv[0];
	ChangeExt(buff,argv[0],CONFIG_TRAILER);
	fpParam = fopen(buff,"r");
	while	(	(  argc  >  1  )
			||	(  fpParam  !=  NULL  ) )	{
		if		(  fpParam  !=  NULL  )	{
			while	(	(  ( c = fgetc(fpParam) )  !=  EOF  )
					&&	(  isspace(c)                       ) )
				;
			if		(  c  !=  EOF  )	{
				p = q = buff;
				do	{
					*q = (char)c;
					q ++;
					c = fgetc(fpParam);
				}	while	(	(  c  !=  '\n'  )
							&&	(  c  !=  EOF   ) );
				*q = 0;
				ungetc(c,fpParam);
			} else {
				p = NULL;
				fclose(fpParam);
				fpParam = NULL;
			}
		} else {
			argv ++;
			p = *argv;
			argc --;
		}
		if		(  p  ==  NULL  )	{
			isParam = TRUE;
		} else
		if		(  p  ==  buff  )	{
			isParam = AnalizeLine(tbl,p);
		} else {
			isParam = TRUE;
			if		(  *p  ==  RESPONSE_FILE_SWITCH  )	{
				LastArg = NULL;
				fpParam = fopen(p+1,"r");
			} else
			if		(  *p  ==  COMMAND_SWITCH  )	{
				LastArg = NULL;
				while	(  *p  ==  COMMAND_SWITCH  )
					p ++;
				if		(	(  !strcmp(p,"?")  )
						||	(  !strcmp(p,"h")  )
						||	(  !strcmp(p,"H")  ) ) {
					sprintf(buff,"USAGE:%s <option(s)> arguments...",cmd);
					PrintUsage(tbl,buff,help);
					exit(0);
				} else {
					isParam = AnalizeLine(tbl,p);
				}
			} else
			if		(  LastArg  !=  NULL  )	{
				isParam = AnalizeLine(tbl,p);
			} else {
				isParam = FALSE;
			}
		}
		if		(  !isParam  ) {
			next = (FILE_LIST *)malloc(sizeof(FILE_LIST));
			next->next = fl;
			next->name = strdup(p);
			fl = next;
		}
	}

	next = NULL;
	while	(  fl  !=  NULL  )	{
		next1 = fl->next;
		fl->next = next;
		next = fl;
		fl = next1;
	}
	return	(next);
}

#ifdef	MAIN
static	char	*a;
static	long	b;
static	Bool	c;
static	ARG_TABLE	option[] = {
	"a",		STRING,		TRUE,	(void*)&a,
		"aaa",
	"b",		INTEGER,	TRUE,	(void*)&b,
		"bbb",
	"c",		BOOLEAN,	TRUE,	(void*)&c,
		"ccc",
	NULL
};
static	void
SetSystemDefault(void)
{
	a = "abc";
	b = 10;
	c = FALSE;
}

main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetSystemDefault();
	fl = GetOption(option,argc,argv);
	printf("*****\n");
	PrintUsage(option,argv[0]);
	if		(  fl  !=  NULL  ) {
		printf("ext = [%s]\n",GetExt(fl->name));
	}
}
#endif
