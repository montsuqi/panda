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
#include	<math.h>

#include	"types.h"
#include	"misc.h"
#include	"value.h"
#include	"glterm.h"
#include	"comm.h"
#include	"debug.h"

#define	NUM_BUFF	60

static	guint
NameHash(
	gconstpointer	key)
{
	char	*p;
	guint	ret;

	ret = 0;
	if		(  key  !=  NULL  ) {
		for	( p = (char *)key ; *p != 0 ; p ++ ) {
			ret = ( ret << 4 ) | *p;
		}
	}
	return	(ret);
}

static	gint
NameCompare(
	gconstpointer	s1,
	gconstpointer	s2)
{
	return	(!strcmp((char *)s1,(char *)s2));
}

extern	GHashTable	*
NewNameHash(void)
{
	GHashTable	*ret;

	ret = g_hash_table_new((GHashFunc)NameHash,(GCompareFunc)NameCompare);

	return	(ret);
}

static	guint
NameiHash(
	gconstpointer	key)
{
	char	*p;
	guint	ret;

	ret = 0;
	if		(  key  !=  NULL  ) {
		for	( p = (char *)key ; *p != 0 ; p ++ ) {
			ret = ( ret << 4 ) | toupper(*p);
		}
	}
	return	(ret);
}

static	gint
NameiCompare(
	gconstpointer	s1,
	gconstpointer	s2)
{
	return	(!stricmp((char *)s1,(char *)s2));
}

extern	GHashTable	*
NewNameiHash(void)
{
	GHashTable	*ret;

	ret = g_hash_table_new((GHashFunc)NameiHash,(GCompareFunc)NameiCompare);

	return	(ret);
}

static	void
DestroySymbols(
	GHashTable		*sym)
{
	void	ClearNames(
		gpointer	key,
		gpointer	value,
		gpointer	user_data)
		{
			xfree(key);
		}

	if		(  sym  !=  NULL  ) {
		g_hash_table_foreach(sym,(GHFunc)ClearNames,NULL);
		g_hash_table_destroy(sym);
	}
}

static	guint
IntHash(
	gconstpointer	key)
{
	return	((guint)key);
}

static	gint
IntCompare(
	gconstpointer	s1,
	gconstpointer	s2)
{
	return	( (int)s1 == (int)s2 );
}

extern	GHashTable	*
NewIntHash(void)
{
	GHashTable	*ret;

	ret = g_hash_table_new((GHashFunc)IntHash,(GCompareFunc)IntCompare);

	return	(ret);
}

extern	void
FreeValueStruct(
	ValueStruct	*val)
{
	int		i;

	if		(  val  !=  NULL  ) {
		switch	(val->type) {
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < val->body.ArrayData.count ; i ++ ) {
				FreeValueStruct(val->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < val->body.RecordData.count ; i ++ ) {
				FreeValueStruct(val->body.RecordData.item[i]);
			}
			DestroySymbols(val->body.RecordData.members);
			break;
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
		  case	GL_TYPE_TEXT:
			if		(  val->body.CharData.sval  !=  NULL  ) {
				xfree(val->body.CharData.sval);
			}
			break;
		  case	GL_TYPE_NUMBER:
			if		(  val->body.FixedData.sval  !=  NULL  ) {
				xfree(val->body.FixedData.sval);
			}
			break;
		  default:
			break;
		}
		xfree(val);
	}
}

extern	ValueStruct	*
GetRecordItem(
	ValueStruct	*value,
	char		*name)
{	
	gpointer	p;
	ValueStruct	*item;

	if		(  value->type  ==  GL_TYPE_RECORD  ) {
		if		(  ( p = g_hash_table_lookup(value->body.RecordData.members,name) )
				   ==  NULL  ) {
			item = NULL;
		} else {
			item = value->body.RecordData.item[(int)p-1];
		}
	} else {
		item = NULL;
	}
	return	(item);
}

extern	ValueStruct	*
GetArrayItem(
	ValueStruct	*value,
	int			i)
{
	ValueStruct	*item;
	
	if		(	(  i  >=  0                            )
			&&	(  i  <   value->body.ArrayData.count  ) )	{
		item = value->body.ArrayData.item[i];
	} else {
		item = NULL;
	}
	return	(item);
}

extern	char	*
ToString(
	ValueStruct	*val)
{
	static	char	buff[SIZE_BUFF];
	char	*p;

	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		strcpy(buff,ValueString(val));
		break;
	  case	GL_TYPE_NUMBER:
		p = ValueFixed(val)->sval;
		if		(  *p  >=  0x70  ) {
			buff[0] = '-';
			*p ^= 0x40;
			strcpy(buff+1,p);
		} else {
			strcpy(buff,p);
		}
		break;
	  case	GL_TYPE_INT:
		sprintf(buff,"%d",ValueInteger(val));
		break;
	  case	GL_TYPE_FLOAT:
		sprintf(buff,"%g",ValueFloat(val));
		break;
	  case	GL_TYPE_BOOL:
		sprintf(buff,"%s",ValueBool(val) ? "TRUE" : "FALSE");
		break;
	  default:
		*buff = 0;
	}
	return	(buff);
}

extern	Bool
SetValueString(
	ValueStruct	*val,
	char		*str)
{
	Bool	rc
	,		fMinus;
	size_t	len;
	char	*p
	,		*q;
	int		i;
	char	buff[SIZE_NAME];

	if		(  val  ==  NULL  ) {
		fprintf(stderr,"no ValueStruct\n");
		return	(FALSE);
	}
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		memclear(val->body.CharData.sval,val->body.CharData.len + 1);
		if		(  str  !=  NULL  ) {
			strncpy(val->body.CharData.sval,str,val->body.CharData.len);
		}
		rc = TRUE;
		break;
	  case	GL_TYPE_NUMBER:
		p = buff;
		fMinus = FALSE;
		while	(  *str  !=  0  ) {
			if		(  *str  ==  '-'  ) {
				fMinus = TRUE;
			} else
			if	(	(  isdigit(*str)  )
				||	(  *str  ==  '.'  ) ) {
				*p ++ = *str;
			}
			str ++;
		}
		*p = 0;
		memclear(val->body.FixedData.sval,val->body.FixedData.flen);
		p = val->body.FixedData.sval + val->body.FixedData.flen;
		*p -- = 0;
		len =  strlen(buff);
		q = buff + len - 1;
		for	( i = 0 ; i < val->body.FixedData.flen ; i ++ , p --, q -- ) {
			if		(  i  <  len  ) {
				*p = *q;
			} else {
				*p = '0';
			}
		}
		if		(  fMinus  ) {
			*val->body.FixedData.sval |= 0x40;
		}
		rc = TRUE;
		break;
	  case	GL_TYPE_TEXT:
		len = strlen(str) + 1;
		if		(  len  !=  val->body.CharData.len  ) {
			if		(  val->body.CharData.sval  !=  NULL  ) {
				xfree(val->body.CharData.sval);
			}
			val->body.CharData.sval = (char *)xmalloc(len + 1);
		}
		strcpy(val->body.CharData.sval,str);
		rc = TRUE;
		break;
	  case	GL_TYPE_INT:
		val->body.IntegerData = atoi(str);
		rc = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		val->body.FloatData = atof(str);
		rc = TRUE;
		break;
	  case	GL_TYPE_BOOL:
		val->body.BoolData = ( *str == 'T' ) ? TRUE : FALSE;
		rc = TRUE;
		break;
	  default:
		rc = FALSE;	  
	}
	return	(rc);
}

extern	Bool
SetValueInteger(
	ValueStruct	*val,
	int			ival)
{
	Bool	rc;
	char	str[SIZE_BUFF];
	Bool	fMinus;

	if		(  val  ==  NULL  ) {
		fprintf(stderr,"no ValueStruct\n");
		return	(FALSE);
	}
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(str,"%d",ival);
		rc = SetValueString(val,str);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  ival  <  0  ) {
			ival = - ival;
			fMinus = TRUE;
		} else {
			fMinus = FALSE;
		}
		sprintf(str,"%0*d",ValueFixed(val)->flen,ival);
		if		(  fMinus  ) {
			*str |= 0x40;
		}
		rc = SetValueString(val,str);
		break;
	  case	GL_TYPE_INT:
		val->body.IntegerData = ival;
		rc = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		val->body.FloatData = ival;
		rc = TRUE;
		break;
	  case	GL_TYPE_BOOL:
		val->body.BoolData = ( ival == 0 ) ? FALSE : TRUE;
		rc = TRUE;
		break;
	  default:
		rc = FALSE;	  
	}
	return	(rc);
}

extern	Bool
SetValueBool(
	ValueStruct	*val,
	Bool		bval)
{
	Bool	rc;
	char	str[SIZE_BUFF];

	if		(  val  ==  NULL  ) {
		fprintf(stderr,"no ValueStruct\n");
		return	(FALSE);
	}
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(str,"%s",(bval ? "TRUE" : "FALSE"));
		rc = SetValueString(val,str);
		break;
	  case	GL_TYPE_NUMBER:
		sprintf(str,"%d",bval);
		rc = SetValueString(val,str);
		break;
	  case	GL_TYPE_INT:
		val->body.IntegerData = bval;
		rc = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		val->body.FloatData = bval;
		rc = TRUE;
		break;
	  case	GL_TYPE_BOOL:
		val->body.BoolData = bval;
		rc = TRUE;
		break;
	  default:
		rc = FALSE;	  
	}
	return	(rc);
}

extern	void
FloatToFixed(
	Fixed	*xval,
	double	fval)
{
	char	str[NUM_BUFF+1];
	char	*p
	,		*q;
	Bool	fMinus;

	if		(  fval  <  0  ) {
		fval = - fval;
		fMinus = TRUE;
	} else {
		fMinus = FALSE;
	}
	sprintf(str, "%0*.*f", (int)xval->flen+1, (int)xval->slen, fval);
	p = str;
	q = xval->sval;
	while	(  *p  !=  0  ) {
		if		( *p  !=  '.'  ) {
			*q = *p;
			q ++;
		}
		p ++;
	}
	if		(  fMinus  ) {
		*xval->sval |= 0x40;
	}
}

extern	Bool
SetValueFloat(
	ValueStruct	*val,
	double		fval)
{
	Bool	rc;
	char	str[SIZE_BUFF];

	if		(  val  ==  NULL  ) {
		fprintf(stderr,"no ValueStruct\n");
		return	(FALSE);
	}
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(str,"%f",fval);
		rc = SetValueString(val,str);
		break;
	  case	GL_TYPE_NUMBER:
		FloatToFixed(ValueFixed(val),fval);
		rc = TRUE;
		break;
	  case	GL_TYPE_INT:
		val->body.IntegerData = fval;
		rc = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		val->body.FloatData = fval;
		rc = TRUE;
		break;
	  case	GL_TYPE_BOOL:
		val->body.BoolData = ( fval == 0 ) ? FALSE : TRUE;
		rc = TRUE;
		break;
	  default:
		rc = FALSE;	  
	}
	return	(rc);
}

static	void
FixedRescale(
	Fixed	*to,
	Fixed	*fr)
{
	char	*p
	,		*q;
	size_t	len;

	memset(to->sval,'0',to->flen);
	to->sval[to->flen] = 0;
	p = fr->sval + ( fr->flen - fr->slen );
	q = to->sval + ( to->flen - to->slen );
	len = ( fr->slen > to->slen ) ? to->slen : fr->slen;
	for	( ; len > 0 ; len -- ) {
		*q ++ = *p ++;
	}
	p = fr->sval + ( fr->flen - fr->slen ) - 1;
	q = to->sval + ( to->flen - to->slen ) - 1;
	len = ( ( fr->flen - fr->slen ) > ( to->flen - to->slen ) )
		? ( to->flen - to->slen ) : ( fr->flen - fr->slen );
	for	( ; len > 0 ; len -- ) {
		*q -- = *p --;
	}
}

extern	int
FixedToInt(
	Fixed	*xval)
{
	int		ival;
	int		i;

	ival = atoi(xval->sval);
	for	( i = 0 ; i < xval->slen ; i ++ ) {
		ival /= 10;
	}
	return	(ival);
}

extern	double
FixedToFloat(
	Fixed	*xval)
{
	double	fval;
	int		i;
	Bool	fMinus;

	if		(  *xval->sval  >=  0x70  ) {
		*xval->sval ^= 0x40;
		fMinus = TRUE;
	} else {
		fMinus = FALSE;
	}
	fval = atof(xval->sval);
	for	( i = 0 ; i < xval->slen ; i ++ ) {
		fval /= 10.0;
	}
	if		(  fMinus  ) {
		fval = - fval;
	}
	return	(fval);
}

extern	Bool
SetValueFixed(
	ValueStruct	*val,
	Fixed		*fval)
{
	Bool	rc;

	if		(  val  ==  NULL  ) {
		fprintf(stderr,"no ValueStruct\n");
		return	(FALSE);
	}
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		rc = SetValueString(val,fval->sval);
		break;
	  case	GL_TYPE_NUMBER:
		FixedRescale(ValueFixed(val),fval);
		rc = TRUE;
		break;
	  case	GL_TYPE_INT:
		val->body.IntegerData = FixedToInt(fval);
		rc = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		val->body.FloatData = FixedToFloat(fval);
		rc = TRUE;
		break;
#if	0
	  case	GL_TYPE_BOOL:
		val->body.BoolData = ( *fval->sval == 0 ) ? FALSE : TRUE;
		rc = TRUE;
		break;
#endif
	  default:
		rc = FALSE;	  
	}
	return	(rc);
}

extern	Numeric
FixedToNumeric(
	Fixed	*xval)
{
	Numeric	value
	,		value2;
	Bool	fMinus;

	if		(  *xval->sval  >=  0x70  ) {
		*xval->sval ^= 0x40;
		fMinus = TRUE;
	} else {
		fMinus = FALSE;
	}
	value = NumericInput(xval->sval,xval->flen,xval->slen);
	value2 = NumericShift(value,- xval->slen);
	NumericFree(value);
	if		(  fMinus  ) {
		value = NumericUMinus(value2);
	} else {
		value = value2;
	}
	return	(value);
}

extern	char	*
NumericToFixed(
	Numeric	value,
	int		precision,
	int		scale)
{
	char	*fr
	,		*to;
	char	*p
	,		*q;
	size_t	len;
	Bool	fMinus;

	fr = NumericOutput(value);
	if		(  *fr  ==  '-'  ) {
		fMinus = TRUE;
		fr ++;
	} else {
		fMinus = FALSE;
	}
	to = (char *)xmalloc(precision+1);
	memset(to,'0',precision);
	to[precision] = 0;
	if		(  ( p = strchr(fr,'.') )  !=  NULL  ) {
		p ++;
		q = to + ( precision - scale );
		len = ( strlen(p) > scale ) ? scale : strlen(p);
		for	( ; len > 0 ; len -- ) {
			*q ++ = *p ++;
		}
	}
	if		(  ( p = strchr(fr,'.') )  !=  NULL  ) {
		p --;
	} else {
		p = fr + strlen(fr) - 1;
	}
	q = to + ( precision - scale ) - 1;
	len = ( ( p - fr + 1 ) > ( precision - scale ) )
		? ( precision - scale ) : ( p - fr + 1 );
	for	( ; len > 0 ; len -- ) {
		*q -- = *p --;
	}
	if		(  fMinus  ) {
		*to |= 0x40;
	}
	return	(to);
}

extern	ValueStruct	*
GetItemLongName(
	ValueStruct		*root,
	char			*longname)
{
	char	item[SIZE_NAME+1]
	,		*p
	,		*q;
	int		n;
	ValueStruct	*val;

dbgmsg(">GetItemLongName");
	if		(  root  ==  NULL  ) { 
		printf("no root ValueStruct [%s]\n",longname);
		return	(FALSE);
	}
	p = longname; 
	val = root;
	while	(  *p  !=  0  ) {
		q = item;
		if		(  *p  ==  '['  ) {
			p ++;
			while	(  isdigit(*p)  ) {
				*q ++ = *p ++;
			}
			if		(  *p  !=  ']'  ) {
				/*	fatal error	*/
			}
			*q = 0;
			p ++;
			n = atoi(item);
			val = GetArrayItem(val,n);
		} else {
			while	(	(  *p  !=  0    )
					&&	(  *p  !=  '.'  )
					&&	(  *p  !=  '['  ) ) {
				*q ++ = *p ++;
			}
			*q = 0;
			val = GetRecordItem(val,item);
		}
		if		(  *p   ==  '.'   )	p ++;
		if		(  val  ==  NULL  )	{
			printf("no ValueStruct [%s]\n",longname);
			break;
		}
	}
dbgmsg("<GetItemLongName");
	return	(val); 
}

static	void
DumpItem(
	gpointer	key,
	gpointer	value,
	gpointer	user_data)
{
	ValueStruct	*val = (ValueStruct *)user_data;
	int			pos = (int)value;
	ValueStruct	*item;

	printf("%s:",(char *)key);
	item = val->body.RecordData.item[pos-1];
	printf("%s:",((item->attr&GL_ATTR_INPUT) == GL_ATTR_INPUT) ? "I" : "O");
	DumpValueStruct(item);
}

extern	void
DumpValueStruct(
	ValueStruct	*val)
{
	int		i;

	if		(  val  ==  NULL  )	{
		printf("null value\n");
	} else
	switch	(val->type) {
	  case	GL_TYPE_INT:
		printf("integer[%d]\n",val->body.IntegerData);
		fflush(stdout);
		break;
	  case	GL_TYPE_FLOAT:
		printf("float[%g]\n",val->body.FloatData);
		fflush(stdout);
		break;
	  case	GL_TYPE_BOOL:
		printf("Bool[%s]\n",(val->body.BoolData ? "T" : "F"));
		fflush(stdout);
		break;
	  case	GL_TYPE_CHAR:
		printf("char(%d) [%s]\n",val->body.CharData.len,val->body.CharData.sval);
		fflush(stdout);
		break;
	  case	GL_TYPE_VARCHAR:
		printf("varchar(%d) [%s]\n",val->body.CharData.len,val->body.CharData.sval);
		fflush(stdout);
		break;
	  case	GL_TYPE_DBCODE:
		printf("code(%d) [%s]\n",val->body.CharData.len,val->body.CharData.sval);
		fflush(stdout);
		break;
	  case	GL_TYPE_NUMBER:
		printf("number(%d,%d) [%s]\n",val->body.FixedData.flen,
			   val->body.FixedData.slen,
			   val->body.FixedData.sval);
		fflush(stdout);
		break;
	  case	GL_TYPE_TEXT:
		printf("text(%d) [%s]\n",val->body.CharData.len,val->body.CharData.sval);
		fflush(stdout);
		break;
	  case	GL_TYPE_ARRAY:
		printf("array size = %d\n",val->body.ArrayData.count);
		fflush(stdout);
		for	( i = 0 ; i < val->body.ArrayData.count ; i ++ ) {
			DumpValueStruct(val->body.ArrayData.item[i]);
		}
		break;
	  case	GL_TYPE_RECORD:
		printf("record members = %d\n",val->body.RecordData.count);
		fflush(stdout);
		g_hash_table_foreach(val->body.RecordData.members,(GHFunc)DumpItem,val);
		printf("--\n");
		break;
	  default:
		break;
	}
}

extern	size_t
SizeValue(
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	int		i
	,		n;
	size_t	ret;

	if		(  val  ==  NULL  )	return	(0);
dbgmsg(">SizeValue");
	switch	(val->type) {
	  case	GL_TYPE_INT:
		ret = sizeof(int);
		break;
	  case	GL_TYPE_FLOAT:
		ret = sizeof(double);
		break;
	  case	GL_TYPE_BOOL:
		ret = 1;
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		ret = val->body.CharData.len;
		break;
	  case	GL_TYPE_NUMBER:
		ret = val->body.FixedData.flen;
		break;
	  case	GL_TYPE_TEXT:
		if		(  textsize  >  0  ) {
			ret = textsize;
		} else {
			ret = val->body.CharData.len + sizeof(size_t);
		}
		break;
	  case	GL_TYPE_ARRAY:
		if		(  val->body.ArrayData.count  >  0  ) {
			n = val->body.ArrayData.count;
		} else {
			n = arraysize;
		}
		ret = SizeValue(val->body.ArrayData.item[0],arraysize,textsize) * n;
		break;
	  case	GL_TYPE_RECORD:
		ret = 0;
		for	( i = 0 ; i < val->body.RecordData.count ; i ++ ) {
			ret += SizeValue(val->body.RecordData.item[i],arraysize,textsize);
		}
		break;
	  default:
		ret = 0;
		break;
	}
dbgmsg("<SizeValue");
	return	(ret);
}

extern	void
InitializeValue(
	ValueStruct	*value)
{
	int		i;

dbgmsg(">InitializeValue");
	if		(  value  ==  NULL  )	return;
	switch	(value->type) {
	  case	GL_TYPE_INT:
		value->body.IntegerData = 0;
		break;
	  case	GL_TYPE_FLOAT:
		value->body.FloatData = 0.0;
		break;
	  case	GL_TYPE_BOOL:
		value->body.BoolData = FALSE;
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		memclear(value->body.CharData.sval,value->body.CharData.len+1);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  value->body.FixedData.flen  >  0  ) {
			memset(value->body.FixedData.sval,'0',value->body.FixedData.flen);
		}
		value->body.FixedData.sval[value->body.FixedData.flen] = 0;
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
			InitializeValue(value->body.ArrayData.item[i]);
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
			InitializeValue(value->body.RecordData.item[i]);
		}
		break;
	  default:
		break;
	}
dbgmsg("<InitializeValue");
}

extern	char	*
UnPackValue(
	char	*p,
	ValueStruct	*value)
{
	int		i;
	size_t	size;

dbgmsg(">UnPackValue");
	if		(  value  !=  NULL  ) {
		switch	(value->type) {
		  case	GL_TYPE_INT:
			memcpy(&value->body.IntegerData,p,sizeof(int));
			p += sizeof(int);
			break;
		  case	GL_TYPE_BOOL:
			value->body.BoolData = ( *(char *)p == 'T' ) ? TRUE : FALSE;
			p ++;
			break;
		  case	GL_TYPE_BYTE:
			memcpy(value->body.CharData.sval,p,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
			//			memcpy(value->body.CharData.sval,p,value->body.CharData.len);
			strcpy(value->body.CharData.sval,p);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_TEXT:
			memcpy(&size,p,sizeof(size_t));
			p += sizeof(size_t);
			if		(  size  !=  value->body.CharData.len  ) {
				xfree(value->body.CharData.sval);
				value->body.CharData.sval = (char *)xmalloc(size+1);
				value->body.CharData.len = size;
			}
			memcpy(value->body.CharData.sval,p,size);
			p += size;
			break;
		  case	GL_TYPE_NUMBER:
			memcpy(value->body.FixedData.sval,p,value->body.FixedData.flen);
			p += value->body.FixedData.flen;
			break;
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
				p = UnPackValue(p,value->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
				p = UnPackValue(p,value->body.RecordData.item[i]);
			}
			break;
		  default:
			break;
		}
	}
dbgmsg("<UnPackValue");
	return	(p);
}

extern	char	*
PackValue(
	char	*p,
	ValueStruct	*value)
{
	int		i;

dbgmsg(">PackValue");
	if		(  value  !=  NULL  ) {
		switch	(value->type) {
		  case	GL_TYPE_INT:
			memcpy(p,&value->body.IntegerData,sizeof(int));
			p += sizeof(int);
			break;
		  case	GL_TYPE_BOOL:
			*(char *)p = value->body.BoolData ? 'T' : 'F';
			p ++;
			break;
		  case	GL_TYPE_BYTE:
			memcpy(p,value->body.CharData.sval,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
			memcpy(p,value->body.CharData.sval,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_TEXT:
			memcpy(p,&value->body.CharData.len,sizeof(size_t));
			p += sizeof(size_t);
			memcpy(p,value->body.CharData.sval,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_NUMBER:
			memcpy(p,ValueFixed(value)->sval,ValueFixed(value)->flen);
			p += value->body.FixedData.flen;
			break;
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
				p = PackValue(p,value->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
				p = PackValue(p,value->body.RecordData.item[i]);
			}
			break;
		  default:
			break;
		}
	}
dbgmsg("<PackValue");
	return	(p);
}


extern	void
DestroyPort(
	Port	*port)
{
	if		(  port->port  !=  NULL  ) {
		xfree(port->port);
	}
	if		(  port->host  !=  NULL  ) {
		xfree(port->host);
	}
	xfree(port);
}

extern	Port	*
ParPort(
	char	*str,
	int		def)
{
	Port	*ret;
	char	*p;
	char	dup[SIZE_BUFF+1];

	strncpy(dup,str,SIZE_BUFF);
	ret = New(Port);
	if		(  dup[0]  ==  '['  ) {
		if		(  ( p = strchr(dup,']') )  !=  NULL  ) {
			*p = 0;
			ret->host = StrDup(&dup[1]);
			p ++;
			if		(  *p  ==  ':'  ) {
				ret->port = StrDup(p+1);
			} else {
				if		(  def  <  0  ) {
					ret->port = NULL;
				} else {
					ret->port = IntStrDup(def);
				}
			}
		}
	} else {
		if		(  ( p = strchr(dup,':') )  !=  NULL  ) {
			*p = 0;
			ret->host = StrDup(dup);
			ret->port = StrDup(p+1);
		} else {
			ret->host = StrDup(dup);
			if		(  def  <  0  ) {
				ret->port = NULL;
			} else {
				ret->port = IntStrDup(def);
			}
		}
	}
	return	(ret);
}

extern	void
ParseURL(
	URL		*url,
	char	*instr)
{
	char	*p
	,		*str;
	char	buff[256];
	Port	*port;

	strcpy(buff,instr);
	str = buff;
	if		(  ( p = strchr(str,':') )  ==  NULL  ) {
		url->protocol = StrDup("http");
	} else {
		*p = 0;
		url->protocol = StrDup(str);
		str = p + 1;
	}
	if		(  *str  ==  '/'  ) {
		str ++;
	}
	if		(  *str  ==  '/'  ) {
		str ++;
	}
	if		(  ( p = strchr(str,'/') )  !=  NULL  ) {
		*p = 0;
	}
	port = ParPort(str,-1);
	url->host = StrDup(port->host);
	url->port = StrDup(port->port);
	DestroyPort(port);
	if		(  p  !=  NULL  ) {
		url->file = StrDup(p+1);
	} else {
		url->file = "";
	}
}

extern	char	**
ParCommandLine(
	char	*line)
{
	int			n;
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;
	char		**cmd;

dbgmsg(">ParCommandLine");
	n = 0;
	p = line;
	while	(  *p  !=  0  ) {
		n ++;
		while	(	(  *p  !=  0  )
				&&	(  !isspace(*p)  ) ) {
			p ++;
		}
		while	(	(  *p  !=  0  )
				&&	(  isspace(*p)  ) ) {
			p ++;
		}
	}
	cmd = (char **)xmalloc(sizeof(char *) * ( n + 1));
	p = line;
	n = 0;
	while	(  *p  !=  0  ) {
		q = buff;
		while	(	(  *p  !=  0  )
				&&	(  !isspace(*p)  ) ) {
			*q ++ = *p ++;
		}
		*q = 0;
		cmd[n] = StrDup(buff);
		n ++;
		while	(	(  *p  !=  0  )
				&&	(  isspace(*p)  ) ) {
			p ++;
		}
	}
	cmd[n] = NULL;
dbgmsg("<ParCommandLine");

	return	(cmd); 
}

extern	char	*
ExpandPath(
	char	*org,
	char	*base)
{
	static	char	path[SIZE_NAME+1];
	char	*p
	,		*q;

	p = path;
	while	(  *org  !=  0  ) {
		if		(  *org  ==  '~'  ) {
			p += sprintf(p,"%s",getenv("HOME"));
		} else
		if		(  *org  ==  '='  ) {
			if		(  base  ==  NULL  ) {
				if		(  ( q = getenv("BASE_DIR") )  !=  NULL  ) {
					p += sprintf(p,"%s",q);
				} else {
					p += sprintf(p,".");
				}
			} else {
				p += sprintf(p,"%s",base);
			}
		} else {
			*p ++ = *org;
		}
		org ++;
	}
	*p = 0;
	return	(path);
}

extern	DB_Func	*
NewDB_Func(void)
{
	DB_Func	*ret;

	ret = New(DB_Func);
	ret->exec = NULL;
	ret->access = NULL;
	ret->table = NewNameHash();
	return	(ret);
}

