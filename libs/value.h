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

#ifndef	_INC_VALUE_H
#define	_INC_VALUE_H

#include	"const.h"
#include	<glib.h>
#include	<numeric.h>
#include	"LBSfunc.h"

typedef	unsigned char	PacketDataType;
typedef	unsigned char	ValueAttributeType;

typedef	struct	{
	char	*protocol;
	char	*host;
	char	*port;
	char	*file;
}	URL;

typedef	struct {
	char	*host;
	char	*port;
}	Port;

typedef	struct {
	size_t		flen;
	size_t		slen;
	char		*sval;
}	Fixed;

typedef	struct _ValueStruct	{
	PacketDataType		type;
	Bool				fUpdate;
	ValueAttributeType	attr;
	union {
		struct {
			size_t					count;
			struct	_ValueStruct	**item;
		}	ArrayData;
		struct {
			size_t					count;
			struct	_ValueStruct	**item;
			char					**names;
			GHashTable				*members;
		}	RecordData;
		struct {
			size_t		len;
			char		*sval;
		}	CharData;
		Fixed	FixedData;
		int		IntegerData;
		Bool	BoolData;
		double	FloatData;
	}	body;
}	ValueStruct;

typedef	struct {
	char		*name;
	Bool		fNew;
	int			PutType;
	ValueStruct	*Value;
}	WindowData;

typedef	struct {
	char	***item;
}	KeyStruct;

#define	DBOP_SELECT		0
#define	DBOP_FETCH		1
#define	DBOP_UPDATE		2
#define	DBOP_INSERT		3
#define	DBOP_DELETE		4

typedef	struct {
	char			*name;
	GHashTable		*opHash;
	int				ocount;
	LargeByteString	**ops;
}	PathStruct;

#define	SIZE_FUNC		16
typedef	struct {
	char	func[SIZE_FUNC];
	int		rc;
	int		blocks;
	int		rno;
	int		pno;
}	DBCOMM_CTRL;

typedef	struct _DBG_Struct	{
	char		*name;
	char		*type;
	struct	_DB_Func		*func;
	FILE		*fpLog;
	LargeByteString	*redirectData;
	char		*file;
	Port		*redirectPort;
	struct	_DBG_Struct	*redirect;
	GHashTable	*dbt;
	int			priority;
	/*	DB depend	*/
	Port		*port;
	char		*dbname;
	char		*user;
	char		*pass;
	/*	DB connection variable	*/
	Bool		fConnect;
	void		*conn;
}	DBG_Struct;

typedef	struct {
	KeyStruct	*pkey;
	PathStruct	**path;
	GHashTable	*paths;
	int			pcount;
	DBG_Struct	*dbg;
}	DB_Struct;

#define	RECORD_NULL		0
#define	RECORD_DB		1

typedef	struct _RecordStruct	{
	char		*name;
	ValueStruct	*rec;
	int		type;
	union {
		DB_Struct	*db;
	}	opt;
}	RecordStruct;

typedef	void	(*DB_FUNC)(DBCOMM_CTRL *, RecordStruct *);
typedef	void	(*DB_EXEC)(DBG_Struct *, char *);
typedef	Bool	(*DB_FUNC_NAME)(char *, DBCOMM_CTRL *, RecordStruct *);

typedef	struct _DB_Func	{
	DB_FUNC_NAME	access;
	DB_EXEC			exec;
	GHashTable		*table;
	char			*commentStart
	,				*commentEnd;
}	DB_Func;

#define	GL_TYPE_NULL			(PacketDataType)0x00
#define	GL_TYPE_INT				(PacketDataType)0x10
#define	GL_TYPE_BOOL			(PacketDataType)0x11
#define	GL_TYPE_FLOAT			(PacketDataType)0x20
#define	GL_TYPE_CHAR			(PacketDataType)0x30
#define	GL_TYPE_TEXT			(PacketDataType)0x31
#define	GL_TYPE_VARCHAR			(PacketDataType)0x32
#define	GL_TYPE_BYTE			(PacketDataType)0x40
#define	GL_TYPE_NUMBER			(PacketDataType)0x50

#define	GL_TYPE_DBCODE			(PacketDataType)0x60

#define	GL_TYPE_STRUCTURE		(PacketDataType)0x80
#define	GL_TYPE_ARRAY			(PacketDataType)0x90
#define	GL_TYPE_RECORD			(PacketDataType)0xA0

#define	GL_ATTR_NULL			(ValueAttributeType)0x00
#define	GL_ATTR_VIRTUAL			(ValueAttributeType)0x80
#define	GL_ATTR_INPUT			(ValueAttributeType)0x08
#define	GL_ATTR_OUTPUT			(ValueAttributeType)0x04

#define	IS_VALUE_RECORD(v)		((v)->type == GL_TYPE_RECORD)
#define	IS_VALUE_STRUCTURE(v)	(((v)->type & GL_TYPE_STRUCTURE) == GL_TYPE_STRUCTURE)

#define	ValueString(v)			((v)->body.CharData.sval)
#define	ValueByte(v)			((v)->body.CharData.sval)
#define	ValueInteger(v)			((v)->body.IntegerData)
#define	ValueBool(v)			((v)->body.BoolData)
#define	ValueFloat(v)			((v)->body.FloatData)
#define	ValueFixed(v)			(&(v)->body.FixedData)

extern	GHashTable	*NewNameHash(void);
extern	GHashTable	*NewNameiHash(void);
extern	GHashTable	*NewIntHash(void);

extern	ValueStruct	*GetRecordItem(ValueStruct *value, char *name);
extern	ValueStruct	*GetArrayItem(ValueStruct *value, int i);
extern	ValueStruct	*GetItemLongName(ValueStruct *root, char *longname);
extern	Bool		SetValueString(ValueStruct *val, char *str);
extern	Bool		SetValueInteger(ValueStruct *val, int ival);
extern	Bool		SetValueBool(ValueStruct *val, Bool bval);
extern	Bool		SetValueFloat(ValueStruct *val, double bval);
extern	Bool		SetValueFixed(ValueStruct *val, Fixed *fval);

extern	int			FixedToInt(Fixed *xval);
extern	void		FloatToFixed(Fixed *xval, double fval);
extern	double		FixedToFloat(Fixed *xval);

extern	Numeric		FixedToNumeric(Fixed *xval);
extern	char		*NumericToFixed(Numeric value, int precision, int scale);

extern	size_t		SizeValue(ValueStruct *val, size_t arraysize, size_t textsize);
extern	void		InitializeValue(ValueStruct *value);
extern	char		*ToString(ValueStruct *value);


extern	void		FreeValueStruct(ValueStruct *val);
extern	void		DumpValueStruct(ValueStruct *val);

extern	char		*UnPackValue(char *p, ValueStruct *value);
extern	char		*PackValue(char *p, ValueStruct *value);

extern	void		ParseURL(URL *url, char *str);
extern	void		DestroyPort(Port *port);
extern	Port		*ParPort(char *str, int def);
extern	char		**ParCommandLine(char *line);
extern	char		*ExpandPath(char *org,char *base);

extern	DB_Func		*NewDB_Func(void);
extern	void		DecodeString(char *q, char *p);
extern	void		EncodeString(char *q, char *p);

#define	g_int_hash_table_insert(h,i,d)		g_hash_table_insert((h),(void *)(i),(d))
#define	g_int_hash_table_foreach(h,p,a)		g_hash_table_foreach((h),(p),(a))
#define	g_int_hash_table_lookup(h,i)		g_hash_table_lookup((h),(void *)(i))
#define	g_int_hash_table_remove(h,i)		g_hash_table_remove((h),(void *)(i))
#endif
