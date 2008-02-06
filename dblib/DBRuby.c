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

#ifdef HAVE_RUBY
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	<numeric.h>

#include    <ruby.h>
#include    <env.h>
#include    <st.h>

#include	<sys/types.h>
#include	<sys/wait.h>

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"dblib.h"
#include	"redirect.h"
#include	"debug.h"

#define	DBIN_FILENO		3
#define	DBOUT_FILENO	4

typedef	struct {
	NETFILE	*fpW;
	NETFILE	*fpR;
	GHashTable	*rhash;
}	DBRubyConn;

static	char	*codeset;

static VALUE application_instances;
static VALUE aryval_new(ValueStruct *val, int need_free);
static VALUE recval_new(ValueStruct *val, int need_free);
static VALUE recval_get_field(VALUE self);
static VALUE recval_set_field(VALUE self, VALUE obj);

static VALUE mPanda;
static VALUE cArrayValue;
static VALUE cRecordValue;
static VALUE eDatabaseError;

#define TAG_RETURN  0x1
#define TAG_BREAK   0x2
#define TAG_NEXT    0x3
#define TAG_RETRY   0x4
#define TAG_REDO    0x5
#define TAG_RAISE   0x6
#define TAG_THROW   0x7
#define TAG_FATAL   0x8
#define TAG_MASK    0xf

typedef struct _protect_call_arg {
    VALUE recv;
    ID mid;
    int argc;
    VALUE *argv;
} protect_call_arg;

static VALUE
protect_funcall0(VALUE arg)
{
    return rb_funcall2(((protect_call_arg *) arg)->recv,
                       ((protect_call_arg *) arg)->mid,
                       ((protect_call_arg *) arg)->argc,
                       ((protect_call_arg *) arg)->argv);
}

static VALUE
rb_protect_funcall(VALUE recv, ID mid, int *state, int argc, ...)
{
    va_list ap;
    VALUE *argv;
    protect_call_arg arg;

    if (argc > 0) {
    int i;

    argv = ALLOCA_N(VALUE, argc);

    va_start(ap, argc);
    for (i = 0; i < argc; i++) {
        argv[i] = va_arg(ap, VALUE);
    }
    va_end(ap);
    }
    else {
    argv = 0;
    }
    arg.recv = recv;
    arg.mid = mid;
    arg.argc = argc;
    arg.argv = argv;
    return rb_protect(protect_funcall0, (VALUE) &arg, state);
}

static void
error_pos()
{
    ruby_set_current_source();
    if (ruby_sourcefile) {
        if (ruby_frame->last_func) {
            fprintf(stderr, "%s:%d:in `%s'", ruby_sourcefile, ruby_sourceline,
                    rb_id2name(ruby_frame->last_func));
        }
        else if (ruby_sourceline == 0) {
            fprintf(stderr, "%s", ruby_sourcefile);
        }
        else {
            fprintf(stderr, "%s:%d", ruby_sourcefile, ruby_sourceline);
        }
    }
}

static VALUE
get_backtrace(info)
    VALUE info;
{
    if (NIL_P(info)) return Qnil;
    return rb_funcall(info, rb_intern("backtrace"), 0);
}

static void
error_print()
{
    VALUE errat = Qnil;     /* OK */
    volatile VALUE eclass, e;
    char *einfo;
    long elen;

    if (NIL_P(ruby_errinfo)) return;

    errat = get_backtrace(ruby_errinfo);
    if (NIL_P(errat)){
        ruby_set_current_source();
        if (ruby_sourcefile)
            fprintf(stderr, "%s:%d", ruby_sourcefile, ruby_sourceline);
        else
            fprintf(stderr, "%d", ruby_sourceline);
    }
    else if (RARRAY(errat)->len == 0) {
        error_pos();
    }
    else {
        VALUE mesg = RARRAY(errat)->ptr[0];

        if (NIL_P(mesg)) error_pos();
        else {
            fwrite(RSTRING(mesg)->ptr, 1, RSTRING(mesg)->len, stderr);
        }
    }

    eclass = CLASS_OF(ruby_errinfo);
    e = rb_obj_as_string(ruby_errinfo);
    einfo = RSTRING(e)->ptr;
    elen = RSTRING(e)->len;
    if (eclass == rb_eRuntimeError && elen == 0) {
        fprintf(stderr, ": unhandled exception\n");
    }
    else {
        VALUE epath;

        epath = rb_class_path(eclass);
        if (elen == 0) {
            fprintf(stderr, ": ");
            fwrite(RSTRING(epath)->ptr, 1, RSTRING(epath)->len, stderr);
        }
        else {
            char *tail  = 0;
            long len = elen;

            if (RSTRING(epath)->ptr[0] == '#') epath = 0;
            tail = memchr(einfo, '\n', elen);
            if (tail) {
                len = tail - einfo;
                tail++;     /* skip newline */
            }
            fprintf(stderr, ": ");
            fwrite(einfo, 1, len, stderr);
            if (epath) {
                fprintf(stderr, " (");
                fwrite(RSTRING(epath)->ptr, 1, RSTRING(epath)->len, stderr);
                fprintf(stderr, ")\n");
            }
            if (tail) {
                fwrite(tail, 1, elen-len-1, stderr);
            }
        }
    }

    if (!NIL_P(errat)) {
        long i;
        struct RArray *ep = RARRAY(errat);

#define TRACE_MAX (TRACE_HEAD+TRACE_TAIL+5)
#define TRACE_HEAD 8
#define TRACE_TAIL 5

        ep = RARRAY(errat);
        for (i=1; i<ep->len; i++) {
            if (TYPE(ep->ptr[i]) == T_STRING) {
                fprintf(stderr, "\tfrom %s\n", RSTRING(ep->ptr[i])->ptr);
            }
            if (i == TRACE_HEAD && ep->len > TRACE_MAX) {
                fprintf(stderr, "\t ... %ld levels...\n",
                        ep->len - TRACE_HEAD - TRACE_TAIL);
                i = ep->len - TRACE_TAIL;
            }
        }
    }
}

static int
error_handle(int ex)
{
    int status = EXIT_FAILURE;

    switch (ex & TAG_MASK) {
    case 0:
        status = EXIT_SUCCESS;
        break;
    case TAG_RETURN:
        error_pos();
        fprintf(stderr, ": unexpected return\n");
        break;
    case TAG_NEXT:
        error_pos();
        fprintf(stderr, ": unexpected next\n");
        break;
    case TAG_BREAK:
        error_pos();
        fprintf(stderr, ": unexpected break\n");
        break;
    case TAG_REDO:
        error_pos();
        fprintf(stderr, ": unexpected redo\n");
        break;
    case TAG_RETRY:
        error_pos();
        fprintf(stderr, ": retry outside of rescue clause\n");
        break;
    case TAG_THROW:
        error_pos();
        fprintf(stderr, ": unexpected throw\n");
        break;
    case TAG_RAISE:
    case TAG_FATAL:
        if (rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
            VALUE st = rb_iv_get(ruby_errinfo, "status");
            status = NUM2INT(st);
        }
        else {
            error_print();
        }
        break;
    default:
        fprintf(stderr, "Unknown longjmp status %d", ex);
        break;
    }
    return status;
}

static VALUE
bigdecimal_new(ValueStruct *val)
{
    VALUE cBigDecimal;
    char *s;

    rb_require("bigdecimal");
    cBigDecimal = rb_const_get(rb_cObject, rb_intern("BigDecimal"));
    s = ValueToString(val, NULL);
    return rb_funcall(cBigDecimal, rb_intern("new"), 2, rb_str_new2(s),
                      INT2NUM((ValueFixedLength(val))));
}


static VALUE
get_value(ValueStruct *val)
{
    if (val == NULL)
        return Qnil;
    if (IS_VALUE_NIL(val))
        return Qnil;
    switch (ValueType(val)) {
    case GL_TYPE_BOOL:
        return ValueBool(val) ? Qtrue : Qfalse;
    case GL_TYPE_INT:
        return INT2NUM(ValueInteger(val));
    case GL_TYPE_FLOAT:
        return rb_float_new(ValueFloat(val));
    case GL_TYPE_NUMBER:
        return bigdecimal_new(val);
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_DBCODE:
    case GL_TYPE_TEXT:
        return rb_str_new2(ValueToString(val, codeset));
    case GL_TYPE_BYTE:
    case GL_TYPE_BINARY:
        if (ValueByte(val) == NULL) {
            return Qnil;
        }
        else {
            return rb_str_new(ValueByte(val), ValueByteLength(val));
        }
    case GL_TYPE_OBJECT:
        return INT2NUM(ValueObjectId(val));
    case GL_TYPE_ARRAY:
        return aryval_new(val, 0);
    case GL_TYPE_RECORD:
        return recval_new(val, 0);
    default:
        rb_raise(rb_eArgError, "unsupported ValueStruct type");
        break;
    }
    return Qnil;                /* not reached */
}

typedef struct _value_struct_data {
    ValueStruct *value;
    VALUE cache;
} value_struct_data;

static void
set_value(ValueStruct *value, VALUE obj)
{
    VALUE class_path, str;

    if (NIL_P(obj)) {
        ValueIsNil(value);
    }
    else {
        ValueIsNonNil(value);
        switch (TYPE(obj)) {
        case T_TRUE:
        case T_FALSE:
            SetValueBool(value, RTEST(obj) ? TRUE : FALSE);
            break;
        case T_FIXNUM:
            SetValueInteger(value, FIX2INT(obj));
            break;
        case T_BIGNUM:
            SetValueInteger(value, NUM2INT(obj));
            break;
        case T_FLOAT:
            SetValueFloat(value, RFLOAT(obj)->value);
            break;
        case T_STRING:
            switch (ValueType(value)) {
            case GL_TYPE_BYTE:
            case GL_TYPE_BINARY:
                SetValueBinary(value, RSTRING(obj)->ptr, RSTRING(obj)->len);
                break;
            default:
                SetValueStringWithLength(value,
                                         RSTRING(obj)->ptr,
                                         RSTRING(obj)->len,
                                         codeset);
                break;
            }
            break;
        default:
            class_path = rb_class_path(CLASS_OF(obj));
            if (strcasecmp(StringValuePtr(class_path), "BigDecimal") == 0) {
                str = rb_funcall(obj, rb_intern("to_s"), 1, rb_str_new2("F"));
            }
            else {
                str = rb_funcall(obj, rb_intern("to_s"), 0);
            }
            SetValueString(value, StringValuePtr(str), codeset);
            break;
        }
    }
}

static int
value_equal(ValueStruct *val, VALUE obj)
{
    if (val == NULL)
        return 0;
    if (IS_VALUE_NIL(val))
        return NIL_P(obj);
    switch (ValueType(val)) {
    case GL_TYPE_BOOL:
        if (ValueBool(val)) {
            return obj == Qtrue;
        }
        else {
            return obj == Qfalse;
        }
    case GL_TYPE_INT:
        return ValueInteger(val) == NUM2INT(obj);
    case GL_TYPE_FLOAT:
        return ValueFloat(val) == NUM2DBL(obj);
    case GL_TYPE_NUMBER:
    {
        VALUE bd = bigdecimal_new(val);
        return RTEST(rb_funcall(bd, rb_intern("=="), 1, obj));
    }
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_DBCODE:
    case GL_TYPE_TEXT:
        return strcmp(ValueToString(val, codeset), StringValuePtr(obj)) == 0;
    case GL_TYPE_BYTE:
    case GL_TYPE_BINARY:
        return memcmp(ValueByte(val), StringValuePtr(obj),
                      ValueByteLength(val)) == 0;
    case GL_TYPE_OBJECT:
        return ValueInteger(val) == NUM2INT(obj);
    default:
        return 0;
    }
}

#define CACHEABLE(val) (val != NULL && \
                        (ValueType(val) == GL_TYPE_ARRAY || \
                         ValueType(val) == GL_TYPE_RECORD))

static void
value_struct_mark(value_struct_data *data)
{
    rb_gc_mark(data->cache);
}

static void
value_struct_free(value_struct_data *data)
{
ENTER_FUNC;
	FreeValueStruct(data->value);
	free(data);
LEAVE_FUNC;
}

static VALUE
aryval_new(ValueStruct *val, int need_free)
{
    VALUE obj;
    value_struct_data *data;

    obj = Data_Make_Struct(cArrayValue, value_struct_data,
                           value_struct_mark,
                           (need_free ?
							(RUBY_DATA_FUNC) value_struct_free :
							(RUBY_DATA_FUNC) free),
                           data);
    data->value = val;
    data->cache = rb_ary_new2(ValueArraySize(val));
    return obj;
}

static VALUE
aryval_length(VALUE self)
{
    value_struct_data *data;

    Data_Get_Struct(self, value_struct_data, data);
    return INT2NUM(ValueArraySize(data->value));
}

static VALUE
aryval_aref(VALUE self, VALUE index)
{
    value_struct_data *data;
    int i = NUM2INT(index);
    VALUE obj;
    ValueStruct *val;

    Data_Get_Struct(self, value_struct_data, data);
    if (i >= 0 && i < RARRAY(data->cache)->len &&
        !NIL_P(RARRAY(data->cache)->ptr[i]))
        return RARRAY(data->cache)->ptr[i];
    val = GetArrayItem(data->value, i);
    if (val == NULL)
        return Qnil;
    obj = get_value(val);
    if (CACHEABLE(val))
        rb_ary_store(data->cache, i, obj);
    return obj;
}

static VALUE
aryval_aset(VALUE self, VALUE index, VALUE obj)
{
    value_struct_data *data;
    int i = NUM2INT(index);
    ValueStruct *val;

    Data_Get_Struct(self, value_struct_data, data);
    val = GetArrayItem(data->value, i);
    if (val == NULL)
        rb_raise(rb_eIndexError, "index out of range: %d", i);
    set_value(val, obj);
    return obj;
}

static VALUE
aryval_index(VALUE self, VALUE obj)
{
    value_struct_data *data;
    int i;

    Data_Get_Struct(self, value_struct_data, data);
    for (i = 0; i < ValueArraySize(data->value); i++) {
        if (value_equal(ValueArrayItem(data->value, i), obj))
            return INT2NUM(i);
    }
    return Qnil;
}

static VALUE
recval_new(ValueStruct *val, int need_free)
{
    VALUE obj;
    value_struct_data *data;
    int i;
    VALUE name;

    obj = Data_Make_Struct(cRecordValue, value_struct_data,
                           value_struct_mark,
                           (need_free ?
							(RUBY_DATA_FUNC) value_struct_free :
							(RUBY_DATA_FUNC) free),
                           data);
    data->value = val;
    data->cache = rb_hash_new();
    for (i = 0; i < ValueRecordSize(val); i++) {
        rb_define_singleton_method(obj, ValueRecordName(val, i),
                                   recval_get_field, 0);
        name = rb_str_new2(ValueRecordName(val, i));
        rb_str_cat2(name, "=");
        rb_define_singleton_method(obj, StringValuePtr(name),
                                   recval_set_field, 1);
    }
    return obj;
}

static VALUE
recval_length(VALUE self)
{
    value_struct_data *data;

    Data_Get_Struct(self, value_struct_data, data);
    return INT2NUM(ValueRecordSize(data->value));
}

static VALUE
recval_aref(VALUE self, VALUE name)
{
    VALUE obj;
    value_struct_data *data;
    ValueStruct *val;

    Data_Get_Struct(self, value_struct_data, data);

    if (!NIL_P(obj = rb_hash_aref(data->cache, name)))
        return obj;

    val = GetItemLongName(data->value, StringValuePtr(name));
    if (val == NULL)
        rb_raise(rb_eArgError, "no such field: %s", StringValuePtr(name));
    obj = get_value(val);
    if (CACHEABLE(val))
        rb_hash_aset(data->cache, name, obj);
    return obj;
}

static VALUE
recval_aset(VALUE self, VALUE name, VALUE obj)
{
    value_struct_data *data;
    ValueStruct *val;

    Data_Get_Struct(self, value_struct_data, data);
    val = GetRecordItem(data->value, StringValuePtr(name));
    if (val == NULL)
        rb_raise(rb_eArgError, "no such field: %s", StringValuePtr(name));
    set_value(val, obj);
    return obj;
}

static VALUE
recval_get_field(VALUE self)
{
    VALUE obj;
    value_struct_data *data;
    ValueStruct *val;
    char *name = rb_id2name(ruby_frame->last_func);

    Data_Get_Struct(self, value_struct_data, data);

    if (!NIL_P(obj = rb_hash_aref(data->cache, rb_str_new2(name))))
        return obj;

    val = GetRecordItem(data->value, name);
    obj = get_value(val);
    if (CACHEABLE(val))
        rb_hash_aset(data->cache, rb_str_new2(name), obj);
    return obj;
}

static VALUE
recval_set_field(VALUE self, VALUE obj)
{
    value_struct_data *data;
    ValueStruct *val;
    char *s = rb_id2name(ruby_frame->last_func);
    VALUE name;

    name = rb_str_new(s, strlen(s) - 1);

    Data_Get_Struct(self, value_struct_data, data);
    val = GetRecordItem(data->value, StringValuePtr(name));
    if (val == NULL)
        rb_raise(rb_eArgError, "no such field: %s", StringValuePtr(name));
    set_value(val, obj);
    return obj;
}

static void
init(
	DBG_Struct	*dbg)
{
    VALUE stack_start;
    void Init_stack(VALUE *);

    ruby_init();
    Init_stack(&stack_start);
    ruby_init_loadpath();
#if	0
	default_load_path = rb_load_path;
	rb_global_variable(&default_load_path);
#endif
    mPanda = rb_define_module("Panda");
    cArrayValue = rb_define_class_under(mPanda, "ArrayValue", rb_cObject);
    rb_define_method(cArrayValue, "length", aryval_length, 0);
    rb_define_method(cArrayValue, "size", aryval_length, 0);
    rb_define_method(cArrayValue, "[]", aryval_aref, 1);
    rb_define_method(cArrayValue, "[]=", aryval_aset, 2);
    rb_define_method(cArrayValue, "index", aryval_index, 1);
    cRecordValue = rb_define_class_under(mPanda, "RecordValue", rb_cObject);
    rb_define_method(cRecordValue, "length", recval_length, 0);
    rb_define_method(cRecordValue, "size", recval_length, 0);
    rb_define_method(cRecordValue, "[]", recval_aref, 1);
    rb_define_method(cRecordValue, "[]=", recval_aset, 2);
    eDatabaseError = rb_define_class_under(mPanda, "DatabaseError",
                                           rb_eStandardError);
    application_instances = rb_hash_new();
    rb_gc_register_address(&application_instances);

    codeset = dbg->coding;
}

#define	CMD_Data		(PacketClass)0x01
#define	CMD_Record		(PacketClass)0x02
#define	CMD_Path		(PacketClass)0x03
#define	CMD_Func		(PacketClass)0x04
#define	CMD_Value		(PacketClass)0x05
#define	CMD_NoValue		(PacketClass)0x06
#define	CMD_Error		(PacketClass)0xFE
#define	CMD_End			(PacketClass)0xFF

extern	ValueStruct	*
SearchFunctionArgument(
	char	*rname,
	char	*pname,
	char	*func)
{
	RecordStruct	*rec;
	ValueStruct		*value;
	PathStruct		*path;
	DB_Operation	*op;
	int			rno
		,		pno
		,		ono;

	value = NULL;
	if		(	(  rname  !=  NULL  )
			&&	(  ( rno = (int)(long)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) ) {
		rec = ThisDB[rno-1];
		value = rec->value;
		if		(	(  pname  !=  NULL  )
				&&	(  ( pno = (int)(long)g_hash_table_lookup(rec->opt.db->paths,
														pname) )  !=  0  ) ) {
			path = rec->opt.db->path[pno-1];
			value = ( path->args != NULL ) ? path->args : value;
			if		(	(  func  !=  NULL  )
					&&	( ( ono = (int)(long)g_hash_table_lookup(path->opHash,func) )  !=  0  ) ) {
				op = path->ops[ono-1];
				value = ( op->args != NULL ) ? op->args : value;
			}
		}
	}
	return	(value);
}

static	void
_DefineClass(
	char			*name,
	RecordStruct	*rec,
	LargeByteString	*src)
{
	char	buff[SIZE_BUFF];
	DB_Struct		*db = rec->opt.db;
	int		i
		,	j;
	char	*pname
		,	*fname;
	LargeByteString	*proc;

	sprintf(buff,"class %c%s\n",toupper(*name),name+1);
	LBS_EmitString(src,buff);
	for	( i = 0 ; i < db->ocount ; i ++ ) {
		fname = db->ops[i]->name;
		if		(  ( proc = db->ops[i]->proc )  !=  NULL  ) {
			if		(  strcmp(fname,"DBOPEN")  ==  0  ) {
				fname = "initialize";
			}
			sprintf(buff,"  def %s\n",fname);
			LBS_EmitString(src,buff);
			LBS_EmitString(src,(char *)LBS_Body(proc));
			LBS_EmitString(src,"\n  end\n");
		}
	}
	for	( i = 0 ; i < db->pcount ; i ++ ) {
		for	( j = 0 ; j < db->path[i]->ocount ; j ++ ) {
			pname = db->path[i]->name;
			fname = db->path[i]->ops[j]->name;
			if		(  ( proc = db->path[i]->ops[j]->proc )  !=  NULL  ) {
				sprintf(buff,"  def %s_%s(%s)\n",pname,fname,name);
				LBS_EmitString(src,buff);
				LBS_EmitString(src,(char *)LBS_Body(proc));
				LBS_EmitString(src,"\n  end\n");
			}
		}
	}
	LBS_EmitString(src,"end\n");
}

static	void
_NewInstance(
	char			*name,
	RecordStruct	*rec)
{
	char	buff[SIZE_BUFF];
    VALUE	app_inst
		,	class_name;
	int		state;

ENTER_FUNC;
	sprintf(buff,"%c%s",toupper(*name),name+1);
	class_name = rb_str_new2(buff);
    app_inst = rb_hash_aref(application_instances, class_name);
	if		(  NIL_P(app_inst)  ) {
		sprintf(buff,"%c%s.new",toupper(*name),name+1);
		app_inst = rb_eval_string_protect(buff, &state);
		if		(	(  state  !=  0  )
				&&	(  error_handle(state)  !=  0  ) ) {
			error_print();
		} else {
			rb_hash_aset(application_instances,class_name,app_inst);
		}
	}
LEAVE_FUNC;
}

static	Bool
InstallDefines(
	DBG_Struct	*dbg)
{
	char	buff[SIZE_BUFF];
	LargeByteString	*src;
	int		state;
	Bool	rc;

ENTER_FUNC;
	src = NewLBS();
	if		(  dbg->dbname  !=  NULL  ) {
		sprintf(buff,"DBNAME = \"%s\"\n",dbg->dbname);
		LBS_EmitString(src,buff);
	}
	if		(  dbg->dbt  !=  NULL  ) {
		g_hash_table_foreach(dbg->dbt,(GHFunc)_DefineClass,src);
	}
	LBS_EmitEnd(src);
#ifdef	DEBUG
	printf("defs------------------\n");
	printf("%s",(char *)LBS_Body(src));
	printf("----------------------\n");
#endif
	rb_eval_string_protect((char *)LBS_Body(src), &state);
	FreeLBS(src);
	if		(  state  ==  0  ) {
		g_hash_table_foreach(dbg->dbt,(GHFunc)_NewInstance,NULL);
		rc = TRUE;
	} else {
		error_print();
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
}

static	void
StartChild(
	DBG_Struct	*dbg)
{
	char	rname[SIZE_LONGNAME+1]
		,	pname[SIZE_LONGNAME+1]
		,	fname[SIZE_LONGNAME+1]
		,	mname[SIZE_LONGNAME+1]
		,	kname[SIZE_LONGNAME+1];
	NETFILE	*fpR
		,	*fpW;
	LargeByteString	*buff;
	ValueStruct	*value;
	int		rc;
	size_t	size;
	VALUE	arg
		,	s
		,	method
		,	klass_name
		,	app_inst;
    int 	state
		,	narg;

ENTER_FUNC;
	fpR = SocketToNet(DBIN_FILENO);
	fpW = SocketToNet(DBOUT_FILENO);
	init(dbg);
	if		(  !InstallDefines(dbg)  ) {
		Error("Ruby error");
	}
	buff = NewLBS();
	while	(  RecvPacketClass(fpR)  ==  CMD_Data  ) {
		switch	(RecvPacketClass(fpR)) {
		  case	CMD_Record:
			RecvString(fpR,rname);
			RecvString(fpR,pname);
			RecvString(fpR,fname);
			if		(  RecvPacketClass(fpR)  ==  CMD_Value  ) {
				value = SearchFunctionArgument(rname,pname,fname);
				RecvLBS(fpR,buff);
				NativeUnPackValue(NULL,LBS_Body(buff),value);
				arg = recval_new(value,0);
				narg = 1;
			} else {
				value = NULL;
				arg = (VALUE)0;
				narg = 0;
			}
			dbgprintf("%s:%s:%s",rname,pname,fname);
			sprintf(kname,"%c%s",toupper(*rname),rname+1);
			klass_name = rb_str_new2(kname);
			app_inst = rb_hash_aref(application_instances, klass_name);
			if (NIL_P(app_inst)) {
				dbgmsg("OTHER");
				rc = MCP_BAD_OTHER;
			} else {
				if		(  *pname  ==  0  ) {
					sprintf(mname,"%s",fname);
				} else {
					sprintf(mname,"%s_%s",pname,fname);
				}
				s = rb_str_new2(mname);
				method = rb_intern(StringValuePtr(s));
				rb_protect_funcall(app_inst,method,&state,narg,arg);
				if		(	(  *pname  !=  0  )
						&&	(  state   !=  0  )
						&&	(  error_handle(state)  !=  0  ) ) {
					dbgmsg("ERR");
					rc = MCP_BAD_OTHER;
				} else {
					dbgmsg("OK");
					rc = MCP_OK;
				}
			}
			if		(  rc  ==  MCP_OK  ) {
				if		(  value  !=  NULL  ) {
					size = NativeSizeValue(NULL,value);
					LBS_ReserveSize(buff,size,FALSE);
					NativePackValue(NULL,LBS_Body(buff),value);
					SendPacketClass(fpW,CMD_Value);
					SendLBS(fpW,buff);
				} else {
					SendPacketClass(fpW,CMD_NoValue);
				}
			} else {
				SendPacketClass(fpW,CMD_Error);
				SendInt(fpW,rc);
			}
			Flush(fpW);
			break;
		}
	}
dbgmsg("*");
	CloseNet(fpR);
	CloseNet(fpW);
	FreeLBS(buff);
	exit(0);
LEAVE_FUNC;
}

static	ValueStruct	*
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int		pid;
	int		pSource[2]
		,	pResult[2]
		,	pError[2]
		,	pDBR[2]
		,	pDBW[2];
	DBRubyConn	*conn;

ENTER_FUNC;
	if ( dbg->fConnect == CONNECT ){
		Warning("database is already connected.");
	}
	pipe(pSource);
	pipe(pResult);
	pipe(pError);
	pipe(pDBR);
	pipe(pDBW);
	if		(  ( pid = fork() )  ==  0  ) {
#if	0
		dup2(pSource[0],STDIN_FILENO);
		dup2(pResult[1],STDOUT_FILENO);
		dup2(pError[1],STDERR_FILENO);
		close(pSource[0]);
		close(pSource[1]);
		close(pResult[0]);
		close(pResult[1]);
		close(pError[0]);
		close(pError[1]);
#endif
		dup2(pDBW[0],DBIN_FILENO);
		dup2(pDBR[1],DBOUT_FILENO);
		close(pDBW[0]);
		close(pDBW[1]);
		close(pDBR[0]);
		close(pDBR[1]);
		StartChild(dbg);
	}
	close(pSource[0]);
	close(pResult[1]);
	close(pError[1]);
	close(pDBR[1]);
	close(pDBW[0]);
#if	0
	close(pSource[1]);
	close(pResult[0]);
	close(pError[0]);
#endif

	conn = New(DBRubyConn);
	OpenDB_RedirectPort(dbg);
	conn->fpR = SocketToNet(pDBR[0]);
	conn->fpW = SocketToNet(pDBW[1]);
	conn->rhash = NewNameHash();
	dbg->conn = (void *)conn;
	dbg->fConnect = CONNECT;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
ExecRuby(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	char			*fname,
	char			*rname,
	char			*pname,
	ValueStruct		*args)
{
	DBRubyConn	*conn = dbg->conn;
	LargeByteString	*buff;
	size_t	size;
	int		rc;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rname  ==  NULL  )	rname = "";
	if		(  pname  ==  NULL  )	pname = "";

	SendPacketClass(conn->fpW,CMD_Data);
	SendPacketClass(conn->fpW,CMD_Record);
	SendString(conn->fpW,rname);
	SendString(conn->fpW,pname);
	SendString(conn->fpW,fname);

	buff = NewLBS();
	ret = NULL;
	if		(  args  !=  NULL  ) {
		size = NativeSizeValue(NULL,args);
		LBS_ReserveSize(buff,size,FALSE);
		NativePackValue(NULL,LBS_Body(buff),args);
		SendPacketClass(conn->fpW,CMD_Value);
		SendLBS(conn->fpW,buff);
	} else {
		SendPacketClass(conn->fpW,CMD_NoValue);
	}
	Flush(conn->fpW);
	switch	(RecvPacketClass(conn->fpR))	{
	  case	CMD_Value:
		RecvLBS(conn->fpR,buff);
		ret = DuplicateValue(args,FALSE);
		NativeUnPackValue(NULL,LBS_Body(buff),ret);
		rc = MCP_OK;
		break;
	  case	CMD_NoValue:
		rc = MCP_OK;
		break;
	  default:
		rc = RecvInt(conn->fpR);
		break;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
	FreeLBS(buff);
LEAVE_FUNC;
	return	(ret);
}

static	Bool
_FreeRecs(
	char	*name,
	ValueStruct	*value)
{
	xfree(name);
	FreeValueStruct(value);
	return	(TRUE);
}

static	ValueStruct	*
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	DBRubyConn	*conn;
	int		status;

ENTER_FUNC;
	if		(  dbg->fConnect == CONNECT ) {
		conn = (DBRubyConn *)dbg->conn;
		CloseDB_RedirectPort(dbg);
		SendPacketClass(conn->fpW,CMD_End);
		CloseNet(conn->fpW);
		CloseNet(conn->fpR);
		g_hash_table_foreach_remove(conn->rhash,(GHRFunc)_FreeRecs,NULL);
		while( waitpid(-1, &status, WNOHANG) > 0 );

		dbg->fConnect = DISCONNECT;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
	return	(NULL);
}

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed)
{
	int			rc = MCP_OK;

	LBS_EmitStart(dbg->checkData);
	LBS_EmitEnd(dbg->checkData);
	return	rc;
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	char			*name,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DB_Struct	*db;
	PathStruct	*path;
	ValueStruct	*ret;

ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		if		(  (int)(long)g_hash_table_lookup(path->opHash,name)  ==  0  ) {
			ctrl->rc = MCP_BAD_FUNC;
		} else {
			ret = ExecRuby(dbg,ctrl,name,rec->name,path->name,args);
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
	return	(ret);
}

static	Bool
_DBRECORD(
	DBG_Struct		*dbg,
	char			*fname,
	RecordStruct	*rec)
{
	DB_Struct	*db;
	Bool	rc;
	ValueStruct	*ret;

ENTER_FUNC;
	dbgprintf("[%s]",fname); 
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = TRUE;
	} else {
		db = rec->opt.db;
		if		(  (int)(long)g_hash_table_lookup(db->opHash,fname)  ==  0  ) {
			rc = FALSE;
		} else {
			rc = TRUE;
			if		(  strcmp(fname,"DBOPEN")  !=  0  ) {
				ret = ExecRuby(dbg,NULL,fname,rec->name,NULL,NULL);
				if		(  ret  !=  NULL  ) {
					FreeValueStruct(ret);
				}
			}
		}
	}
LEAVE_FUNC;
	return	(rc);
}

static	ValueStruct	*
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)_DBCOMMIT },

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	_DBRECORD
};

extern	DB_Func	*
InitDBRuby(void)
{
	return	(EnterDB_Function("Ruby",Operations,DB_PARSER_SCRIPT,&Core,"/*","*/\t"));
}

#endif /* #ifdef HAVE_RUBY */
