/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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

#ifdef	HAVE_RUBY
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<dlfcn.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<pthread.h>
#include	<glib.h>

#include	"ruby.h"
#include	"env.h"

#include	"types.h"
#include	"const.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"load.h"
#include	"dbgroup.h"
#include	"queue.h"
#include	"driver.h"
#include	"apslib.h"
#include	"debug.h"


static VALUE application_classes;
static char	*load_path;
static char *codeset;

static VALUE mPanda;
static VALUE cArrayValue;
static VALUE cRecordValue;
static VALUE cRecordStruct;
static VALUE cProcessNode;

#define TAG_RETURN	0x1
#define TAG_BREAK	0x2
#define TAG_NEXT	0x3
#define TAG_RETRY	0x4
#define TAG_REDO	0x5
#define TAG_RAISE	0x6
#define TAG_THROW	0x7
#define TAG_FATAL	0x8
#define TAG_MASK	0xf

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
    VALUE errat = Qnil;		/* OK */
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
                tail++;		/* skip newline */
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
get_value(ValueStruct *val)
{
    if (val == NULL)
        return Qnil;
    switch (ValueType(val)) {
    case GL_TYPE_BOOL:
		return ValueBool(val) ? Qtrue : Qfalse;
    case GL_TYPE_INT:
		return INT2NUM(ValueInteger(val));
    case GL_TYPE_FLOAT:
		return rb_float_new(ValueFloat(val));
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
        return Data_Wrap_Struct(cArrayValue, NULL, NULL, val);
    case GL_TYPE_RECORD:
        return Data_Wrap_Struct(cRecordValue, NULL, NULL, val);
    default:
        rb_raise(rb_eArgError, "unsupported ValueStruct type");
        break;
    }
    return Qnil;                /* not reached */
}

static void
set_value(ValueStruct *value, VALUE obj)
{
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
        rb_raise(rb_eArgError, "unsupported type: %d",
                 rb_class2name(CLASS_OF(obj)));
        break;
    }
}

static VALUE
aryval_length(VALUE self)
{
    ValueStruct *value;

    Data_Get_Struct(self, ValueStruct, value);
    return INT2NUM(ValueArraySize(value));
}

static VALUE
aryval_aget(VALUE self, VALUE index)
{
    ValueStruct *value;
    int i = NUM2INT(index);

    Data_Get_Struct(self, ValueStruct, value);
    if (i >= 0 && i < ValueArraySize(value)) {
        return get_value(ValueArrayItem(value, i));
	}
    else {
		return Qnil;
	}
}

static VALUE
aryval_aset(VALUE self, VALUE index, VALUE obj)
{
    ValueStruct *value;
    int i = NUM2INT(index);

    Data_Get_Struct(self, ValueStruct, value);
    if (i >= 0 && i < ValueArraySize(value)) {
        set_value(ValueArrayItem(value, i), obj);
	}
    else {
		rb_raise(rb_eIndexError, "index out of range: %d", i);
	}
    return obj;
}

static VALUE
recval_length(VALUE self)
{
    ValueStruct *value;

    Data_Get_Struct(self, ValueStruct, value);
    return INT2NUM(ValueRecordSize(value));
}

static VALUE
recval_aget(VALUE self, VALUE name)
{
    ValueStruct *value, *val;

    Data_Get_Struct(self, ValueStruct, value);
    val = GetRecordItem(value, StringValuePtr(name));
    return get_value(val);
}

static VALUE
recval_aset(VALUE self, VALUE name, VALUE obj)
{
    ValueStruct *value, *val;

    Data_Get_Struct(self, ValueStruct, value);
    val = GetRecordItem(value, StringValuePtr(name));
    if (val == NULL)
        rb_raise(rb_eArgError, "no such field: %s", StringValuePtr(name));
    set_value(val, obj);
    return obj;
}

static VALUE
rec_new(RecordStruct *rec)
{
    return Data_Wrap_Struct(cRecordStruct, NULL, NULL, rec);
}

static VALUE
rec_name(VALUE self)
{
    RecordStruct *rec;

    Data_Get_Struct(self, RecordStruct, rec);
    return rb_str_new2(rec->name);
}

static VALUE
rec_length(VALUE self)
{
    RecordStruct *rec;

    Data_Get_Struct(self, RecordStruct, rec);
    return INT2NUM(ValueRecordSize(rec->value));
}

static VALUE
rec_aget(VALUE self, VALUE name)
{
    RecordStruct *rec;
    ValueStruct *val;

    Data_Get_Struct(self, RecordStruct, rec);
    val = GetRecordItem(rec->value, StringValuePtr(name));
    return get_value(val);
}

static VALUE
rec_aset(VALUE self, VALUE name, VALUE obj)
{
    RecordStruct *rec;
    ValueStruct *val;

    Data_Get_Struct(self, RecordStruct, rec);
    val = GetRecordItem(rec->value, StringValuePtr(name));
    if (val == NULL)
        rb_raise(rb_eArgError, "no such field: %s", StringValuePtr(name));
    set_value(val, obj);
    return obj;
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
                           procnode_mark, NULL, data);
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

static void
init()
{
    VALUE stack_start;
    void Init_stack(VALUE *);

    ruby_init();
    Init_stack(&stack_start);
    ruby_init_loadpath();

    mPanda = rb_define_module("Panda");
    cArrayValue = rb_define_class_under(mPanda, "ArrayValue", rb_cObject);
    rb_define_method(cArrayValue, "length", aryval_length, 0);
    rb_define_method(cArrayValue, "size", aryval_length, 0);
    rb_define_method(cArrayValue, "[]", aryval_aget, 1);
    rb_define_method(cArrayValue, "[]=", aryval_aset, 2);
    cRecordValue = rb_define_class_under(mPanda, "RecordValue", rb_cObject);
    rb_define_method(cRecordValue, "length", recval_length, 0);
    rb_define_method(cRecordValue, "size", recval_length, 0);
    rb_define_method(cRecordValue, "[]", recval_aget, 1);
    rb_define_method(cRecordValue, "[]=", recval_aset, 2);
    cRecordStruct = rb_define_class_under(mPanda, "RecordStruct", rb_cObject);
    rb_define_method(cRecordStruct, "length", rec_length, 0);
    rb_define_method(cRecordStruct, "size", rec_length, 0);
    rb_define_method(cRecordStruct, "[]", rec_aget, 1);
    rb_define_method(cRecordStruct, "[]=", rec_aset, 2);
    cProcessNode = rb_define_class_under(mPanda, "ProcessNode", rb_cObject);
    rb_define_method(cProcessNode, "mcp", procnode_mcp, 0);
    rb_define_method(cProcessNode, "link", procnode_link, 0);
    rb_define_method(cProcessNode, "spa", procnode_spa, 0);
    rb_define_method(cProcessNode, "windows", procnode_windows, 0);

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
load_application(char *path, char *name)
{
    VALUE app_class, class_name;
    char *filename;
    char *p;
    int state;

    class_name = rb_str_new2(name);
    app_class = rb_hash_aref(application_classes, class_name);
	if (NIL_P(app_class)) {
        filename = (char *) alloca(strlen(name) + 4);
		sprintf(filename, "%s.rb", name);
        p = filename;
        while (*p) {
            if (isupper(*p))
                *p = tolower(*p);
            p++;
        }

        rb_load_protect(rb_str_new2(filename), 0, &state);
        if (state && error_handle(state))
            return Qnil;
        app_class = rb_eval_string_protect(name, &state);
        if (state && error_handle(state))
            return Qnil;
        rb_hash_aset(application_classes, class_name, app_class);
	}
	return app_class;
}

static	Bool
execute_dc(MessageHandler *handler, ProcessNode *node)
{
	VALUE app_class;
	VALUE app;
	ValueStruct *module_longname;
	char *module;
	Bool	rc;
    int state;

    codeset = ConvCodeset(handler->conv);
    module_longname = GetItemLongName(node->mcprec->value, "dc.module");
    module = ValueStringPointer(module_longname);
    app_class = load_application(handler->loadpath, module);
    if (NIL_P(app_class)) {
		fprintf(stderr, "%s is not found\n", module);
		return FALSE;
    }

    app = rb_protect_funcall(app_class, rb_intern("new"), &state, 0);
    if (state && error_handle(state))
        return FALSE;
    rb_protect_funcall(app, rb_intern("start"), &state, 1, procnode_new(node));
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

static	void
ready_execute(MessageHandler *handler)
{
	if (handler->loadpath == NULL) {
		handler->loadpath = load_path;
	}
}

static	MessageHandlerClass	Handler = {
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

extern	MessageHandlerClass	*
Ruby(void)
{
	return (&Handler);
}
#endif
