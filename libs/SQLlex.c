/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	"types.h"
#include	"hash.h"
#include	"libmondai.h"
#include	"Lex.h"
#include	"SQLlex.h"
#include	"debug.h"

#define	GetChar(fp)		fgetc(fp)
#define	UnGetChar(fp,c)	ungetc((c),(fp))

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
							(gpointer)tokentable[i].token);
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
		ret = (int)p;
	} else {
		ret = T_SYMBOL;
	}
	return	(ret);
}

extern	int
SQL_Lex(
	Bool	fName)
{	int		c
	,		len;
	int		token;
	char	*s;

dbgmsg(">SQL_Lex");
  retry: 
	while	(  isspace( c = GetChar(CURR->fp) ) ) {
		if		(  c  ==  '\n'  ) {
			c = ' ';
			CURR->cLine ++;
		}
	}
	if		(	(  c  ==  '!'  )
			||	(  c  ==  '#'  ) ) {
		while	(  ( c = GetChar(CURR->fp) )  !=  '\n'  );
		CURR->cLine ++;
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = CURR->Symbol;
		len = 0;
		while	(  ( c = GetChar(CURR->fp) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(CURR->fp);
			}
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
		}
		*s = 0;
		token = T_SCONST;
	} else
	if		(  c  ==  '\''  ) {
		s = CURR->Symbol;
		len = 0;
		while	(  ( c = GetChar(CURR->fp) )  !=  '\''  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(CURR->fp);
			}
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
		}
		*s = 0;
		token = T_SCONST;
	} else
	if		(	(  isalpha(c)  )
			||	(  isdigit(c) ) )	{
		s = CURR->Symbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(CURR->fp);
		}	while	(	(  isalpha(c)  )
					||	(  isdigit(c)  )
					||	(  c  ==  '_'  ) );
		*s = 0;
		UnGetChar(CURR->fp,c);
		if		(  fName  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(CURR->Symbol);
		}
	} else {
		switch	(c) {
		  case	EOF:
			token = T_EOF;
			break;
		  default:
			token = c;
			break;
		}
	}
#ifdef	TRACE
	if		(  token  >  0x7F  ) {
		printf("DB_Token = [%X][%s]\n",token,CURR->Symbol);
	} else {
		printf("DB_Token = [%c]\n",token);
	}
#endif
dbgmsg("<SQL_Lex");
	return	(token);
}

