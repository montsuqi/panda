/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Shugo Maeda

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
#define DEBUG
#define TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef  HAVE_RUBY
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <fcntl.h>
#include    <dlfcn.h>
#include    <sys/time.h>
#include    <sys/wait.h>
#include    <unistd.h>
#include    <ctype.h>
#include    <pthread.h>
#include    <glib.h>

#include    <ruby.h>
#include    <env.h>
#include    <st.h>

#include    "types.h"
#include    "const.h"
#include    "libmondai.h"
#include    "comm.h"
#include    "directory.h"
#include    "handler.h"
#include    "defaults.h"
#include    "enum.h"
#include    "dblib.h"
#include    "load.h"
#include    "dbgroup.h"
#include    "queue.h"
#include    "driver.h"
#include    "apslib.h"
#include    "debug.h"

EXTERN VALUE rb_load_path;

static VALUE application_classes;
static char *load_path;
static char *codeset;
static VALUE default_load_path;

static VALUE mPanda;
static VALUE cArrayValue;
static VALUE cRecordValue;
static VALUE cRecordStruct;
static VALUE cProcessNode;
static VALUE cPath;
static VALUE cTable;
static VALUE cDatabase;
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
            if (tail = memchr(einfo, '\n', elen)) {
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
    s = ValueToString(val, codeset);
    return rb_funcall(cBigDecimal, rb_intern("new"), 2, rb_str_new2(s),
                      INT2NUM((ValueFixedLength(val))));
}

static VALUE
get_value(ValueStruct *val)
{
    static VALUE aryval_new(ValueStruct *val, int need_free);
    static VALUE recval_new(ValueStruct *val, int need_free);

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

#define CACHEABLE(val) (ValueType(val) == GL_TYPE_ARRAY || \
                        ValueType(val) == GL_TYPE_RECORD)

typedef struct _value_struct_data {
    ValueStruct *value;
    VALUE cache;
} value_struct_data;

static void
value_struct_mark(value_struct_data *data)
{
    rb_gc_mark(data->cache);
}

static void
value_struct_free(value_struct_data *data)
{
    FreeValueStruct(data->value);
    free(data);
}

static VALUE
aryval_new(ValueStruct *val, int need_free)
{
    VALUE obj;
    value_struct_data *data;

    obj = Data_Make_Struct(cArrayValue, value_struct_data,
                           value_struct_mark,
                           need_free ? value_struct_free : free,
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
recval_new(ValueStruct *val, int need_free)
{
    VALUE obj;
    value_struct_data *data;
    int i;
    VALUE name;
    static VALUE recval_get_field(VALUE self);
    static VALUE recval_set_field(VALUE self, VALUE obj);

    obj = Data_Make_Struct(cRecordValue, value_struct_data,
                           value_struct_mark,
                           need_free ? value_struct_free : free,
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

    val = GetRecordItem(data->value, StringValuePtr(name));
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

typedef struct _record_struct_data {
    RecordStruct *rec;
    VALUE value;
} record_struct_data;

#define RECORD_STRUCT(x) (((struct _record_struct_data *) x)->rec)

static void
rec_mark(record_struct_data *data)
{
    rb_gc_mark(data->value);
}

static VALUE
rec_new(RecordStruct *rec)
{
    VALUE obj;
    record_struct_data *data;
    int i;
    VALUE name;
    static VALUE rec_get_field(VALUE self);
    static VALUE rec_set_field(VALUE self, VALUE obj);

    obj = Data_Make_Struct(cRecordStruct, record_struct_data,
                           rec_mark, free, data);
    data->value = recval_new(rec->value, 0);
    for (i = 0; i < ValueRecordSize(rec->value); i++) {
        rb_define_singleton_method(obj, ValueRecordName(rec->value, i),
                                   rec_get_field, 0);
        name = rb_str_new2(ValueRecordName(rec->value, i));
        rb_str_cat2(name, "=");
        rb_define_singleton_method(obj, StringValuePtr(name),
                                   rec_set_field, 1);
    }
    return obj;
}

static VALUE
rec_name(VALUE self)
{
    record_struct_data *data;

    Data_Get_Struct(self, record_struct_data, data);
    return rb_str_new2(RECORD_STRUCT(data)->name);
}

static VALUE
rec_length(VALUE self)
{
    record_struct_data *data;

    Data_Get_Struct(self, record_struct_data, data);
    return recval_length(data->value);
}

static VALUE
rec_aref(VALUE self, VALUE name)
{
    record_struct_data *data;

    Data_Get_Struct(self, record_struct_data, data);
    return recval_aref(data->value, name);
}

static VALUE
rec_aset(VALUE self, VALUE name, VALUE obj)
{
    record_struct_data *data;

    Data_Get_Struct(self, record_struct_data, data);
    return recval_aset(data->value, name, obj);
}

static VALUE
rec_get_field(VALUE self)
{
    record_struct_data *data;

    Data_Get_Struct(self, record_struct_data, data);
    return recval_get_field(data->value);
}

static VALUE
rec_set_field(VALUE self, VALUE obj)
{
    record_struct_data *data;

    Data_Get_Struct(self, record_struct_data, data);
    return recval_set_field(data->value, obj);
}

typedef struct _procnode_data {
    ProcessNode *node;
    VALUE mcp;
    VALUE link;
    VALUE spa;
    VALUE windows;
} procnode_data;

static void
procnode_mark(procnode_data *data)
{
    if (data == NULL) return;
    rb_gc_mark(data->mcp);
    rb_gc_mark(data->link);
    rb_gc_mark(data->spa);
    rb_gc_mark(data->windows);
}

static VALUE
procnode_new(ProcessNode *node)
{
    VALUE obj;
    procnode_data *data;
    int i;

    obj = Data_Make_Struct(cProcessNode, procnode_data,
                           procnode_mark, free, data);
    data->node = node;
    data->mcp = rec_new(node->mcprec);
    data->link = rec_new(node->linkrec);
    data->spa = rec_new(node->sparec);
    data->windows = rb_hash_new();
    for (i = 0; i < node->cWindow; i++) {
        if (node->scrrec[i] != NULL &&
            node->scrrec[i]->value != NULL) {
            VALUE rec = rec_new(node->scrrec[i]);
            rb_hash_aset(data->windows,
                         rb_str_new2(node->scrrec[i]->name), rec);
        }
    }
    return obj;
}

static VALUE
procnode_term(VALUE self)
{
    procnode_data *data;

    Data_Get_Struct(self, procnode_data, data);
    return rb_str_new2(data->node->term);
}

static VALUE
procnode_user(VALUE self)
{
    procnode_data *data;

    Data_Get_Struct(self, procnode_data, data);
    return rb_str_new2(data->node->user);
}

static VALUE
procnode_mcp(VALUE self)
{
    procnode_data *data;

    Data_Get_Struct(self, procnode_data, data);
    return data->mcp;
}

static VALUE
procnode_link(VALUE self)
{
    procnode_data *data;

    Data_Get_Struct(self, procnode_data, data);
    return data->link;
}

static VALUE
procnode_spa(VALUE self)
{
    procnode_data *data;

    Data_Get_Struct(self, procnode_data, data);
    return data->spa;
}

static VALUE
procnode_windows(VALUE self)
{
    procnode_data *data;

    Data_Get_Struct(self, procnode_data, data);
    return data->windows;
}

static VALUE
procnode_put_window(int argc, VALUE *argv, VALUE self)
{
    procnode_data *data;
    VALUE win, type;
    ValueStruct *dc, *window, *widget, *puttype, *status, *rc;

    Data_Get_Struct(self, procnode_data, data);
    rb_scan_args(argc, argv, "02", &type, &win);

    dc = GetRecordItem(data->node->mcprec->value, "dc");
    window = GetRecordItem(dc, "window");
    if (!NIL_P(win)) {
        SetValueString(window, StringValuePtr(win), codeset);
    }
    widget = GetRecordItem(dc, "widget");
    SetValueString(widget, "", codeset);
    puttype = GetRecordItem(dc, "puttype");
    if (NIL_P(type)) {
        SetValueString(puttype, "CURRENT", codeset);
    }
    else {
        SetValueString(puttype, StringValuePtr(type), codeset);
    }
    status = GetRecordItem(dc, "status");
    SetValueString(status, "PUTG", codeset);
    rc = GetRecordItem(data->node->mcprec->value, "rc");
    SetValueInteger(rc, 0);
    return Qnil;
}

typedef struct _path_data {
    PathStruct *path;
    VALUE args;
    VALUE op_args;
} path_data;

static void
path_mark(path_data *data)
{
    rb_gc_mark(data->args);
}

static VALUE
path_new(PathStruct *path)
{
    VALUE obj;
    path_data *data;
    int i;
    VALUE name;
    static VALUE rec_get_field(VALUE self);
    static VALUE rec_set_field(VALUE self, VALUE obj);

    obj = Data_Make_Struct(cPath, path_data,
                           path_mark, free, data);
    data->path = path;
    data->args = path->args ? recval_new(path->args, 0) : Qnil;
    data->op_args = rb_hash_new();
    for (i = 0; i < ValueRecordSize(path->args); i++) {
        rb_define_singleton_method(obj, ValueRecordName(path->args, i),
                                   rec_get_field, 0);
        name = rb_str_new2(ValueRecordName(path->args, i));
        rb_str_cat2(name, "=");
        rb_define_singleton_method(obj, StringValuePtr(name),
                                   rec_set_field, 1);
    }
    return obj;
}

static VALUE
path_name(VALUE self)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return rb_str_new2(data->path->name);
}

static VALUE
path_args(VALUE self)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return data->args;
}

static VALUE
path_length(VALUE self)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return recval_length(data->args);
}

static VALUE
path_aref(VALUE self, VALUE name)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return recval_aref(data->args, name);
}

static VALUE
path_aset(VALUE self, VALUE name, VALUE obj)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return recval_aset(data->args, name, obj);
}

static VALUE
path_op_args(VALUE self, VALUE name)
{
    path_data *data;
    VALUE op_args;
    int no;

    Data_Get_Struct(self, path_data, data);

    if (!NIL_P(op_args = rb_hash_aref(data->op_args, name))) {
        return op_args;
    }

    no = (int) g_hash_table_lookup(data->path->opHash, StringValuePtr(name));
    if (no == 0) {
        rb_raise(rb_eArgError,
                 "no such operation: %s", StringValuePtr(name));
    }
    if (data->path->ops[no - 1]->args == NULL) {
        return Qnil;
    }
    op_args = recval_new(data->path->ops[no - 1]->args, 0);
    rb_hash_aset(data->op_args, name, op_args);
    return op_args;
}

static VALUE
path_get_field(VALUE self)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return recval_get_field(data->args);
}

static VALUE
path_set_field(VALUE self, VALUE obj)
{
    path_data *data;

    Data_Get_Struct(self, path_data, data);
    return recval_set_field(data->args, obj);
}

typedef struct _table_data {
    record_struct_data rec;
    int no;
    VALUE paths;
} table_data;

static VALUE
table_mark(table_data *data)
{
    rec_mark(&data->rec);
    rb_gc_mark(data->paths);
}

static VALUE
table_new(int no, RecordStruct *rec)
{
    VALUE obj;
    table_data *data;
    int i;
    VALUE name;

    obj = Data_Make_Struct(cTable, table_data,
                           table_mark, free, data);
    data->rec.rec = rec;
    data->rec.value = recval_new(rec->value, 0);
    data->no = no;
    data->paths = rb_hash_new();
    for (i = 0; i < ValueRecordSize(rec->value); i++) {
        rb_define_singleton_method(obj, ValueRecordName(rec->value, i),
                                   rec_get_field, 0);
        name = rb_str_new2(ValueRecordName(rec->value, i));
        rb_str_cat2(name, "=");
        rb_define_singleton_method(obj, StringValuePtr(name),
                                   rec_set_field, 1);
    }
    return obj;
}

typedef struct _set_param_arg {
    VALUE hash;
    ValueStruct *value;
} set_param_arg;

static int
set_param(VALUE key, VALUE value, set_param_arg *arg)
{
    st_table *tbl = RHASH(arg->hash)->tbl;
    struct st_table_entry **bins = tbl->bins;
    char *longname;
    ValueStruct *val;

    if (key == Qundef) return ST_CONTINUE;
    longname = StringValuePtr(key);
    val = GetItemLongName(arg->value, longname);
    if (val == NULL) {
        rb_raise(rb_eArgError, "no such parameter: %s", longname);
    }
    set_value(val, value);
    if (RHASH(arg->hash)->tbl != tbl || RHASH(arg->hash)->tbl->bins != bins){
        rb_raise(rb_eIndexError, "rehash occurred during iteration");
    }
    return ST_CONTINUE;
}

static VALUE
table_exec(int argc, VALUE *argv, VALUE self)
{
    table_data *data;
    VALUE funcname, pathname, params;
    char *func, *pname;
    DBCOMM_CTRL ctrl;
    PathStruct *path;
    int no;
    size_t size;
    ValueStruct *value;
    static VALUE table_path(VALUE self, VALUE name);

    Data_Get_Struct(self, table_data, data);
    rb_scan_args(argc, argv, "12", &funcname, &pathname, &params);
    func = StringValuePtr(funcname);
    if (NIL_P(pathname)) {
        pname = NULL;
    }
    else {
        pname = StringValuePtr(pathname);
    }

    ctrl.rno = data->no;
    ctrl.pno = 0;
    ctrl.blocks = 0;

    value = RECORD_STRUCT(data)->value;
    if (pname != NULL) {
        no = (int) g_hash_table_lookup(RECORD_STRUCT(data)->opt.db->paths, pname);
        if (no != 0) {
            PathStruct *path;
            ctrl.pno = no - 1;
            path = RECORD_STRUCT(data)->opt.db->path[ctrl.pno];
            if (path != NULL) {
                if (path->args != NULL) {
                    value = path->args;
                }
                no = (int) g_hash_table_lookup(path->opHash, func);
                if (no != 0) {
                    DB_Operation *operation = path->ops[no - 1];
                    if (operation != NULL && operation->args != NULL) {
                        value = operation->args;
                    }
                }
            }
        }
    }

    if (argc == 3) {
        set_param_arg arg;

        Check_Type(params, T_HASH);
        arg.hash = params;
        arg.value = value;
        st_foreach(RHASH(params)->tbl, set_param, (st_data_t) &arg);
    }

    size = NativeSizeValue(NULL, RECORD_STRUCT(data)->value);
    ctrl.blocks = ((size + sizeof(DBCOMM_CTRL)) / SIZE_BLOCK) + 1;
    strcpy(ctrl.func, func);
    ExecDB_Process(&ctrl, RECORD_STRUCT(data), value);
    if (ctrl.rc == MCP_OK) {
        ValueStruct *result;

        result = DuplicateValue(value);
        CopyValue(result, value);
        return recval_new(result, 1);
    }
    else if (ctrl.rc == MCP_EOF) {
        return Qnil;
    }
    else {
        rb_raise(eDatabaseError, "database error (ctrl.rc=%d)", ctrl.rc);
        return Qnil;            /* not reached */
    }
}

static VALUE
exec_function(char *func, int argc, VALUE *argv, VALUE self)
{
    int exec_argc = argc + 1;
    VALUE *exec_argv;

    exec_argv = ALLOCA_N(VALUE, exec_argc);
    exec_argv[0] = rb_str_new2(func);
    memcpy(exec_argv + 1, argv, sizeof(VALUE) * argc);
    return table_exec(exec_argc, exec_argv, self);
}

static VALUE
table_select(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBSELECT", argc, argv, self);
}

static VALUE
table_fetch(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBFETCH", argc, argv, self);
}

static VALUE
table_insert(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBINSERT", argc, argv, self);
}

static VALUE
table_update(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBUPDATE", argc, argv, self);
}

static VALUE
table_delete(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBDELETE", argc, argv, self);
}

static VALUE
table_get(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBGET", argc, argv, self);
}

static VALUE
table_close_cursor(int argc, VALUE *argv, VALUE self)
{
    return exec_function("DBCLOSECURSOR", argc, argv, self);
}

static VALUE
table_each(int argc, VALUE *argv, VALUE self)
{
    VALUE val;
    int n = (argc > 2 ? 2 : argc);

    table_select(argc, argv, self);
    while (1) {
        val = table_fetch(n, argv, self);
        if (NIL_P(val))
            break;
        rb_yield(val);
    }
    table_close_cursor(n, argv, self);
    return Qnil;
}

static VALUE
table_path(VALUE self, VALUE name)
{
    table_data *data;
    VALUE path;
    int no;

    Data_Get_Struct(self, table_data, data);

    if (!NIL_P(path = rb_hash_aref(data->paths, name))) {
        return path;
    }

    no = (int) g_hash_table_lookup(RECORD_STRUCT(data)->opt.db->paths,
                                   StringValuePtr(name));
    if (no == 0) {
        rb_raise(rb_eArgError,
                 "no such path: %s", StringValuePtr(name));
    }

    path = path_new(RECORD_STRUCT(data)->opt.db->path[no - 1]);
    rb_hash_aset(data->paths, name, path);
    return path;
}

typedef struct _database_data {
    GHashTable *indices;
    RecordStruct **tables;
    VALUE cache;
} database_data;

static void
database_mark(database_data *data)
{
    rb_gc_mark(data->cache);
}

static VALUE
database_new(GHashTable *indices, RecordStruct **tables)
{
    VALUE obj;
    database_data *data;

    obj = Data_Make_Struct(cDatabase, database_data,
                           database_mark, free, data);
    data->indices = indices;
    data->tables = tables;
    data->cache = rb_hash_new();
    return obj;
}

static VALUE
database_aref(VALUE self, VALUE name)
{
    VALUE obj;
    database_data *data;
    ValueStruct *val;
    int no, table_no;
    RecordStruct *rec;

    Data_Get_Struct(self, database_data, data);

    if (!NIL_P(obj = rb_hash_aref(data->cache, name)))
        return obj;

    no = (int) g_hash_table_lookup(data->indices,
                                   StringValuePtr(name));
    if (no == 0) {
        return Qnil;
    }

    table_no = no - 1;
    rec = data->tables[table_no];
    obj = table_new(table_no, rec);
    rb_hash_aset(data->cache, name, obj);
    return obj;
}

static void
init()
{
    VALUE stack_start;
    void Init_stack(VALUE *);

    ruby_init();
    Init_stack(&stack_start);
    ruby_init_loadpath();
	default_load_path = rb_load_path;
	rb_global_variable(&default_load_path);

    mPanda = rb_define_module("Panda");
    cArrayValue = rb_define_class_under(mPanda, "ArrayValue", rb_cObject);
    rb_define_method(cArrayValue, "length", aryval_length, 0);
    rb_define_method(cArrayValue, "size", aryval_length, 0);
    rb_define_method(cArrayValue, "[]", aryval_aref, 1);
    rb_define_method(cArrayValue, "[]=", aryval_aset, 2);
    cRecordValue = rb_define_class_under(mPanda, "RecordValue", rb_cObject);
    rb_define_method(cRecordValue, "length", recval_length, 0);
    rb_define_method(cRecordValue, "size", recval_length, 0);
    rb_define_method(cRecordValue, "[]", recval_aref, 1);
    rb_define_method(cRecordValue, "[]=", recval_aset, 2);
    cRecordStruct = rb_define_class_under(mPanda, "RecordStruct", rb_cObject);
    rb_define_method(cRecordStruct, "name", rec_name, 0);
    rb_define_method(cRecordStruct, "length", rec_length, 0);
    rb_define_method(cRecordStruct, "size", rec_length, 0);
    rb_define_method(cRecordStruct, "[]", rec_aref, 1);
    rb_define_method(cRecordStruct, "[]=", rec_aset, 2);
    cProcessNode = rb_define_class_under(mPanda, "ProcessNode", rb_cObject);
    rb_define_method(cProcessNode, "term", procnode_term, 0);
    rb_define_method(cProcessNode, "user", procnode_user, 0);
    rb_define_method(cProcessNode, "mcp", procnode_mcp, 0);
    rb_define_method(cProcessNode, "link", procnode_link, 0);
    rb_define_method(cProcessNode, "spa", procnode_spa, 0);
    rb_define_method(cProcessNode, "windows", procnode_windows, 0);
    rb_define_method(cProcessNode, "put_window", procnode_put_window, -1);
    cPath = rb_define_class_under(mPanda, "Path", rb_cObject);
    rb_define_method(cPath, "name", path_name, 0);
    rb_define_method(cPath, "args", path_args, 0);
    rb_define_method(cPath, "length", path_length, 0);
    rb_define_method(cPath, "size", path_length, 0);
    rb_define_method(cPath, "[]", path_aref, 1);
    rb_define_method(cPath, "[]=", path_aset, 2);
    rb_define_method(cPath, "op_args", path_op_args, 1);
    cTable = rb_define_class_under(mPanda, "Table", rb_cObject);
    rb_define_method(cTable, "name", rec_name, 0);
    rb_define_method(cTable, "length", rec_length, 0);
    rb_define_method(cTable, "size", rec_length, 0);
    rb_define_method(cTable, "[]", rec_aref, 1);
    rb_define_method(cTable, "[]=", rec_aset, 2);
    rb_define_method(cTable, "exec", table_exec, -1);
    rb_define_method(cTable, "select", table_select, -1);
    rb_define_method(cTable, "fetch", table_fetch, -1);
    rb_define_method(cTable, "insert", table_insert, -1);
    rb_define_method(cTable, "update", table_update, -1);
    rb_define_method(cTable, "delete", table_delete, -1);
    rb_define_method(cTable, "get", table_get, -1);
    rb_define_method(cTable, "close_cursor", table_close_cursor, -1);
    rb_define_method(cTable, "each", table_each, -1);
    rb_define_method(cTable, "path", table_path, 1);
    cDatabase = rb_define_class_under(mPanda, "Database", rb_cObject);
    rb_define_method(cDatabase, "[]", database_aref, 1);
    eDatabaseError = rb_define_class_under(mPanda, "DatabaseError",
                                           rb_eStandardError);

    application_classes = rb_hash_new();
    rb_gc_register_address(&application_classes);
    if (LibPath == NULL) {
        load_path = getenv("APS_RUBY_PATH");
    }
    else {
        load_path = LibPath;
    }
    codeset = "utf-8";
}

static VALUE
get_source_filename(char *class_name, char *path)
{
    VALUE basename, filename;
    int i;

    if (!isupper(*class_name)) {
        return Qnil;
    }

    basename = rb_str_new2(class_name);
    for (i = 0; i < RSTRING(basename)->len; i++) {
        if (isupper(RSTRING(basename)->ptr[i]))
            RSTRING(basename)->ptr[i] = tolower(RSTRING(basename)->ptr[i]);
    }
    filename = rb_str_new2(path);
    rb_str_cat(filename, "/", 1);
    rb_str_concat(filename, basename); 
    rb_str_cat(filename, ".rb", 3);
    return filename;
}

static VALUE
load_application(char *path, char *name)
{
    VALUE app_class, class_name;
    VALUE filename;
    char *p;
    int state;

    class_name = rb_str_new2(name);
    app_class = rb_hash_aref(application_classes, class_name);
    if (NIL_P(app_class)) {
        filename = get_source_filename(name, path);
        if (NIL_P(filename)) {
            fprintf(stderr, "invalid module name: %s\n", name);
            return Qnil;
        }
        
        rb_load_protect(filename, 0, &state);
        if (state && error_handle(state))
            return Qnil;
        app_class = rb_eval_string_protect(name, &state);
        if (state && error_handle(state))
            return Qnil;
        rb_hash_aset(application_classes, class_name, app_class);
    }
    return app_class;
}

static  Bool
execute_dc(MessageHandler *handler, ProcessNode *node)
{
    VALUE app_class;
    VALUE app;
    ValueStruct *dc_module_value, *dc_status_value;
    char *dc_module, *dc_status;
    Bool    rc;
    int state;
    ID handler_method;
    int i;

    if (handler->loadpath == NULL) {
        fprintf(stderr, "loadpath is required\n");
        return FALSE;
    }
    rb_load_path = rb_ary_new();
    rb_ary_push(rb_load_path, rb_str_new2(handler->loadpath));
    for (i = 0; i < RARRAY(default_load_path)->len; i++) {
        rb_ary_push(rb_load_path,
                    rb_str_dup(RARRAY(default_load_path)->ptr[i]));
    }
    codeset = ConvCodeset(handler->conv);
    dc_module_value = GetItemLongName(node->mcprec->value, "dc.module");
    dc_module = ValueStringPointer(dc_module_value);
    app_class = load_application(handler->loadpath, dc_module);
    if (NIL_P(app_class)) {
        fprintf(stderr, "%s is not found\n", dc_module);
        return FALSE;
    }
    dc_status_value = GetItemLongName(node->mcprec->value, "dc.status");
    dc_status = ValueStringPointer(dc_status_value);
    if (strcmp(dc_status, "LINK") == 0) {
        handler_method = rb_intern("start");
    }
    else if (strcmp(dc_status, "PUTG") == 0) {
        VALUE s;
        ValueStruct *dc_event_value;
        char *dc_event;
        dc_event_value = GetItemLongName(node->mcprec->value, "dc.event");
        dc_event = ValueStringPointer(dc_event_value);
        s = rb_str_new2("do_");
        rb_str_cat2(s, dc_event);
        handler_method = rb_intern(StringValuePtr(s));
    }
    app = rb_protect_funcall(app_class, rb_intern("new"), &state, 0);
    if (state && error_handle(state))
        return FALSE;
    rb_protect_funcall(app, handler_method, &state,
                       2, procnode_new(node), database_new(DB_Table, ThisDB));
    if (state && error_handle(state))
        return FALSE;
    if (ValueInteger(GetItemLongName(node->mcprec->value, "rc")) < 0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static void
ready_dc(MessageHandler *handler)
{
    init();
}

static int
execute_batch(MessageHandler *handler, char *name, char *param)
{
    VALUE app_class;
    VALUE app;
    char *module_longname;
    char *module;
    VALUE rc;
    int state;

    init();
    app_class = load_application_class(handler->loadpath, name);
    if (NIL_P(app_class)) {
        fprintf(stderr, "%s is not found.", module);
        return FALSE;
    }

    app = rb_protect_funcall(app_class, rb_intern("new"), &state, 0);
    if (state && error_handle(state))
        return FALSE;
    rc = rb_protect_funcall(app, rb_intern("start"), &state,
                            1, rb_str_new2(param));
    if (state && error_handle(state))
        return FALSE;
    return NUM2INT(rc);
}

static  void
ready_execute(MessageHandler *handler)
{
    if (handler->loadpath == NULL) {
        handler->loadpath = load_path;
    }
}

static  MessageHandlerClass Handler = {
    "Ruby",
    ready_execute,
    execute_dc,
    execute_batch,
    ready_dc,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

extern  MessageHandlerClass *
Ruby(void)
{
    return (&Handler);
}
#endif
