/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"Lex.h"
#include	"SQLlex.h"
#include	"debug.h"

extern	void
UnGetChar(
	CURFILE	*in,
	int		c)
{
	in->pos --;
	in->back = c;
}

extern	int
GetChar(
	CURFILE	*in)
{
	int		c;

	if		(  in->back  >=  0  ) {
		c = in->back;
		in->back = -1;
	} else {
		if		(  in->pos  ==  in->size  ) {
			c = 0;
		} else
		if		(  in->fp  !=  NULL  ) {
			if		(  ( c = fgetc(in->fp) )  <  0  ) {
				c = 0;
			}
		} else {
			if		(  in->body  ==  NULL  ) {
				fprintf(stderr,"nulpo!\n");
			}
			if		(  ( c = in->body[in->pos] )  ==  0  ) {
				c = 0;
			}
		}
	}
	in->pos ++;
	return	(c);
}

static	TokenTable	tokentable[] = {
	/*	SQL92	*/
	{	"ABSOLUTE",		T_SQL	},
	{	"ACTION",		T_SQL	},
	{	"ADD",			T_SQL	},
	{	"ALL",			T_SQL	},
	{	"ALTER",		T_SQL	},
	{	"AND",			T_SQL	},
	{	"ANY",			T_SQL	},
	{	"AS",			T_SQL	},
	{	"ASC",			T_SQL	},
	{	"AT",			T_SQL 	},
	{	"BEGIN_TRANS",	T_SQL	},
	{	"BETWEEN",		T_SQL	},
	{	"BOTH",			T_SQL	},
	{	"BY",			T_SQL	},
	{	"CASCADE",		T_SQL	},
	{	"CASE",			T_SQL	},
	{	"CAST",			T_SQL	},
	{	"CHAR",			T_SQL	},
	{	"CHARACTER",	T_SQL	},
	{	"CHECK",		T_SQL	},
	{	"CLOSE",		T_SQL	},
	{	"COALESCE",		T_SQL	},
	{	"COLLATE",		T_SQL	},
	{	"COLUMN",		T_SQL	},
	{	"COMMIT",		T_SQL	},
	{	"CONSTRAINT",	T_SQL	},
	{	"CONSTRAINTS",	T_SQL	},
	{	"CREATE",		T_SQL	},
	{	"CROSS",		T_SQL	},
	{	"CURRENT_DATE",	T_SQL	},
	{	"CURRENT_TIME",	T_SQL	},
	{	"CURRENT_TIMESTAMP",	T_SQL	},
	{	"CURRENT_USER",			T_SQL	},
	{	"CURSOR",		T_SQL	},
	{	"DAY_P",		T_SQL	},
	{	"DEC",			T_SQL	},
	{	"DECIMAL",		T_SQL	},
	{	"DECLARE",		T_SQL	},
	{	"DEFAULT",		T_SQL	},
	{	"DELETE",		T_SQL	},
	{	"DESC",			T_SQL	},
	{	"DISTINCT",		T_SQL	},
	{	"DOUBLE",		T_SQL	},
	{	"DROP",			T_SQL	},
	{	"ELSE",			T_SQL	},
	{	"END_TRANS",	T_SQL	},
	{	"ESCAPE",		T_SQL	},
	{	"EXCEPT",		T_SQL	},
	{	"EXECUTE",		T_SQL	},
	{	"EXISTS",		T_SQL	},
	{	"EXTRACT",		T_SQL	},
	{	"FALSE_P",		T_SQL	},
	{	"FETCH",		T_SQL	},
	{	"FLOAT",		T_SQL	},
	{	"FOR",			T_SQL	},
	{	"FOREIGN",		T_SQL	},
	{	"FROM",			T_SQL	},
	{	"FULL",			T_SQL	},
	{	"GLOBAL",		T_SQL	},
	{	"GRANT",		T_SQL	},
	{	"GROUP",		T_SQL	},
	{	"HAVING",		T_SQL	},
	{	"HOUR_P",		T_SQL	},
	{	"IN",			T_SQL	},
	{	"INNER",		T_SQL	},
	{	"INNER_P",		T_SQL	},
	{	"INSENSITIVE",	T_SQL	},
	{	"INSERT",		T_INSERT	},
	{	"INTERSECT",	T_SQL	},
	{	"INTERVAL",		T_SQL	},
	{	"INTO",			T_INTO	},
	{	"IS",			T_SQL	},
	{	"ISOLATION",	T_SQL	},
	{	"JOIN",			T_SQL	},
	{	"KEY",			T_SQL	},
	{	"LANGUAGE",		T_SQL	},
	{	"LEADING",		T_SQL	},
	{	"LEFT",			T_SQL	},
	{	"LEVEL",		T_SQL	},
	{	"LIKE",			T_LIKE	},
	{	"LOCAL",		T_SQL	},
	{	"MATCH",		T_SQL	},
	{	"MINUTE_P",		T_SQL	},
	{	"MONTH_P",		T_SQL	},
	{	"NAMES",		T_SQL	},
	{	"NATIONAL",		T_SQL	},
	{	"NATURAL",		T_SQL	},
	{	"NCHAR",		T_SQL	},
	{	"NEXT",			T_SQL	},
	{	"NO",			T_SQL	},
	{	"NOT",			T_SQL	},
	{	"NULLIF",		T_SQL	},
	{	"NULL_P",		T_SQL	},
	{	"NUMERIC",		T_SQL	},
	{	"OF",			T_SQL	},
	{	"OLD",			T_SQL	},
	{	"ON",			T_SQL	},
	{	"ONLY",			T_SQL	},
	{	"OPTION",		T_SQL	},
	{	"OR",			T_SQL	},
	{	"ORDER",		T_SQL	},
	{	"OUTER_P",		T_SQL	},
	{	"OVERLAPS",		T_SQL	},
	{	"PARTIAL",		T_SQL	},
	{	"POSITION",		T_SQL	},
	{	"PRECISION",	T_SQL	},
	{	"PRIMARY",		T_SQL	},
	{	"PRIOR",		T_SQL	},
	{	"PRIVILEGES",	T_SQL	},
	{	"PROCEDURE",	T_SQL	},
	{	"PUBLIC",		T_SQL	},
	{	"READ",			T_SQL	},
	{	"REFERENCES",	T_SQL	},
	{	"RELATIVE",		T_SQL	},
	{	"REVOKE",		T_SQL	},
	{	"RIGHT",		T_SQL	},
	{	"ROLLBACK",		T_SQL	},
	{	"SCHEMA",		T_SQL	},
	{	"SCROLL",		T_SQL	},
	{	"SECOND_P",		T_SQL	},
	{	"SELECT",		T_SQL	},
	{	"SESSION",		T_SQL	},
	{	"SESSION_USER",	T_SQL	},
	{	"SET",			T_SQL	},
	{	"SOME",			T_SQL	},
	{	"SUBSTRING",	T_SQL	},
	{	"TABLE",		T_SQL	},
	{	"TEMPORARY",	T_SQL	},
	{	"THEN",			T_SQL	},
	{	"TIME",			T_SQL	},
	{	"TIMESTAMP",	T_SQL	},
	{	"TIMEZONE_HOUR",T_SQL	},
	{	"TIMEZONE_MINUTE",	T_SQL	},
	{	"TO",			T_SQL	},
	{	"TRAILING",		T_SQL	},
	{	"TRANSACTION",	T_SQL	},
	{	"TRIM",			T_SQL	},
	{	"TRUE_P",		T_SQL	},
	{	"UNION",		T_SQL	},
	{	"UNIQUE",		T_SQL	},
	{	"UPDATE",		T_SQL	},
	{	"USER",			T_SQL	},
	{	"USING",		T_SQL	},
	{	"VALUES",		T_SQL	},
	{	"VARCHAR",		T_SQL	},
	{	"VARYING",		T_SQL	},
	{	"VIEW",			T_SQL	},
	{	"WHEN",			T_SQL	},
	{	"WHERE",		T_SQL	},
	{	"WITH",			T_SQL	},
	{	"WORK",			T_SQL	},
	{	"YEAR_P",		T_SQL	},
	{	"ZONE",			T_SQL	},
	/*	SQL3	*/
	{	"CHAIN",		T_SQL	},
	{	"CHARACTERISTICS",	T_SQL	},
	{	"DEFERRABLE",	T_SQL	},
	{	"DEFERRED",		T_SQL	},
	{	"IMMEDIATE",	T_SQL	},
	{	"INITIALLY",	T_SQL	},
	{	"INOUT",		T_SQL	},
	{	"OFF",			T_SQL	},
	{	"OUT",			T_SQL	},
	{	"PATH_P",		T_SQL	},
	{	"PENDANT",		T_SQL	},
	{	"RESTRICT",		T_SQL	},
	{	"TRIGGER",		T_SQL	},
	{	"WITHOUT",		T_SQL	},
	/*	SQL92 non reserved	*/
	{	"COMMITTED",	T_SQL	},
	{	"SERIALIZABLE",	T_SQL	},
	{	"TYPE_P",		T_SQL 	},
	/*	Postgres	*/
	{	"ABORT_TRANS",	T_SQL	},
	{	"ACCESS",		T_SQL	},
	{	"AFTER",		T_SQL	},
	{	"AGGREGATE",	T_SQL	},
	{	"ANALYSE",		T_SQL	},
	{	"ANALYZE",		T_SQL	},
	{	"BACKWARD",		T_SQL	},
	{	"BEFORE",		T_SQL	},
	{	"BINARY",		T_SQL	},
	{	"BIT",			T_SQL	},
	{	"CACHE",		T_SQL	},
	{	"CHECKPOINT",	T_SQL	},
	{	"CLUSTER",		T_SQL	},
	{	"COMMENT",		T_SQL	},
	{	"COPY",			T_SQL	},
	{	"CREATEDB",		T_SQL	},
	{	"CREATEUSER",	T_SQL	},
	{	"CYCLE",		T_SQL	},
	{	"DATABASE",		T_SQL	},
	{	"DELIMITERS",	T_SQL	},
	{	"DO",			T_SQL	},
	{	"EACH",			T_SQL	},
	{	"ENCODING",		T_SQL	},
	{	"EXCLUSIVE",	T_SQL	},
	{	"EXPLAIN",		T_SQL	},
	{	"EXTEND",		T_SQL	},
	{	"FORCE",		T_SQL	},
	{	"FORWARD",		T_SQL	},
	{	"FUNCTION",		T_SQL	},
	{	"HANDLER",		T_SQL	},
	{	"ILIKE",		T_ILIKE	},
	{	"INCREMENT",	T_SQL	},
	{	"INDEX",		T_SQL	},
	{	"INHERITS",		T_SQL	},
	{	"INSTEAD",		T_SQL	},
	{	"ISNULL",		T_SQL	},
	{	"LANCOMPILER",	T_SQL	},
	{	"LIMIT",		T_SQL	},
	{	"LISTEN",		T_SQL	},
	{	"LOAD",			T_SQL	},
	{	"LOCATION",		T_SQL	},
	{	"LOCK_P",		T_SQL	},
	{	"MAXVALUE",		T_SQL	},
	{	"MINVALUE",		T_SQL	},
	{	"MODE",			T_SQL	},
	{	"MOVE",			T_SQL	},
	{	"NEW",			T_SQL	},
	{	"NOCREATEDB",	T_SQL	},
	{	"NOCREATEUSER",	T_SQL	},
	{	"NONE",			T_SQL	},
	{	"NOTHING",		T_SQL	},
	{	"NOTIFY",		T_SQL	},
	{	"NOTNULL",		T_SQL	},
	{	"OFFSET",		T_SQL	},
	{	"OIDS",			T_SQL	},
	{	"OPERATOR",		T_SQL	},
	{	"OWNER",		T_SQL	},
	{	"PASSWORD",		T_SQL	},
	{	"PROCEDURAL",	T_SQL	},
	{	"REINDEX",		T_SQL	},
	{	"RENAME",		T_SQL	},
	{	"RESET",		T_SQL	},
	{	"RETURNS",		T_SQL	},
	{	"ROW",			T_SQL	},
	{	"RULE",			T_SQL	},
	{	"SEQUENCE",		T_SQL	},
	{	"SERIAL",		T_SQL	},
	{	"SETOF",		T_SQL	},
	{	"SHARE",		T_SQL	},
	{	"SHOW",			T_SQL	},
	{	"START",		T_SQL	},
	{	"STATEMENT",	T_SQL	},
	{	"STDIN",		T_SQL	},
	{	"STDOUT",		T_SQL	},
	{	"SYSID",		T_SQL	},
	{	"TEMP",			T_SQL	},
	{	"TEMPLATE",		T_SQL	},
	{	"TOAST",		T_SQL	},
	{	"TRUNCATE",		T_SQL	},
	{	"TRUSTED",		T_SQL	},
	{	"UNLISTEN",		T_SQL	},
	{	"UNTIL",		T_SQL	},
	{	"VACUUM",		T_SQL	},
	{	"VALID",		T_SQL	},
	{	"VERBOSE",		T_SQL	},
	{	"VERSION",		T_SQL	},
	{	""				,0		}
};

static	GHashTable	*Reserved;

extern	void
SQL_LexInit(void)
{
	int		i;

	Reserved = NewNameHash();
	for	( i = 0 ; tokentable[i].token  !=  0 ; i ++ ) {
		g_hash_table_insert(Reserved,StrDup(tokentable[i].str),
							(gpointer)(long)tokentable[i].token);
	}
}

static	int
CheckReserved(
	char	*str)
{
	gpointer	p;
	int		ret;
	char	ustr[SIZE_BUFF];
	char	*s;
	
	s = ustr;
	while	(  ( *s = toupper(*str) )  !=  0  ) {
		s ++;
		str ++;
	}
	if		(  ( p = g_hash_table_lookup(Reserved,ustr) ) !=  NULL  ) {
		ret = (int)(long)p;
	} else {
		ret = T_SYMBOL;
	}
	return	(ret);
}

extern	int
SQL_Lex(
	CURFILE	*in,
	Bool	fName)
{
	int		c;
	char	*p;
	char	buff[SIZE_BUFF];

ENTER_FUNC;
  retry: 
	if		(  in->Symbol  !=  NULL  ) {
		xfree(in->Symbol);
		in->Symbol = NULL;
	}
	while	(	(  ( c = GetChar(in) )  !=  0  )
			&&	(  isspace(c)                  ) )	{
		if		(  c  ==  '\n'  ) {
			c = ' ';
			in->cLine ++;
		}
	}
	if		(  c  ==  '#'  ) {
		while	(	(  ( c = GetChar(in) )  !=  0    )
				&&	(  ( c = GetChar(in) )  !=  '\n' ) );
		in->cLine ++;
		goto	retry;
	}
	switch	(c) {
	  case	'"':
		p = buff;
		while	(  ( c = GetChar(in) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(in);
			}
			*p ++ = c;
		}
		*p = 0;
		in->Symbol = (char *)xmalloc(strlen(buff)+1);
		strcpy(in->Symbol,buff);
		in->Token = T_SCONST;
		break;
	  case	'\'':
		p = buff;
		while	(  ( c = GetChar(in) )  !=  '\''  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(in);
			}
			*p ++ = c;
		}
		*p = 0;
		in->Symbol = (char *)xmalloc(strlen(buff)+1);
		strcpy(in->Symbol,buff);
		in->Token = T_SCONST;
		break;
	  default:
		p = buff;
		if		(	(  isalpha(c)  )
				||	(  isdigit(c) ) )	{
			do {
				*p ++ = c;
				c = GetChar(in);
			}	while	(	(  isalpha(c)  )
						||	(  isdigit(c)  )
						||	(  c  ==  '_'  ) );
			UnGetChar(in,c);
			*p = 0;
			in->Symbol = (char *)xmalloc(strlen(buff)+1);
			strcpy(in->Symbol,buff);
			if		(  fName  ) {
				in->Token = T_SYMBOL;
			} else {
				in->Token = CheckReserved(in->Symbol);
			}
		} else {
			switch	(c) {
			  case	EOF:
				in->Token = T_EOF;
				break;
			  default:
				in->Token = c;
				break;
			}
		}
		break;
	}
#ifdef	TRACE
	if		(  in->Token  >  0x7F  ) {
		printf("DB_Token = [%X][%s]\n",in->Token,in->Symbol);
	} else {
		printf("DB_Token = [%c]\n",in->Token);
	}
#endif
LEAVE_FUNC;
	return	(in->Token);
}

