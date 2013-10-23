/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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

#if 0
#define	DEBUG
#define	TRACE
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef	HAVE_RUBY
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>

#include	<ruby.h>

#include	"const.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"queue.h"
#include	"MONFUNC.h"
#include	"message.h"
#include	"debug.h"

static	VALUE	node_info;
static	VALUE	application_classes;

static	CONVOPT	*RubyConv;

#define TAG_RETURN  0x1
#define TAG_BREAK   0x2
#define TAG_NEXT    0x3
#define TAG_RETRY   0x4
#define TAG_REDO    0x5
#define TAG_RAISE   0x6
#define TAG_THROW   0x7
#define TAG_FATAL   0x8
#define TAG_MASK    0xf

static
VALUE
monfunc(
	VALUE self,
	VALUE imcp,
	VALUE idata)
{
	VALUE omcp;
	VALUE odata;
	int rc;
	char *cstr_mcp,*cstr_data;
ENTER_FUNC;
	if (NIL_P(imcp)) {
		Warning("Invalid MCP VALUE");
		return rb_ary_new3(3,INT2NUM(MCP_BAD_ARG),Qnil,Qnil);
	}
	if (NIL_P(idata)) {
		idata = rb_str_new2("");
	}
	rc = RubyMONFUNC(
			StringValueCStr(imcp),
			StringValueCStr(idata),
			&cstr_mcp,
			&cstr_data);
	omcp = rb_str_new2(cstr_mcp);
	xfree(cstr_mcp);
	if (cstr_data != NULL) {
		odata = rb_str_new2(cstr_data);
		xfree(cstr_data);
	} else {
		odata = Qnil;
	}
LEAVE_FUNC;
	return rb_ary_new3(3,INT2NUM(rc),omcp,odata);
}

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
	} else {
		argv = 0;
	}
	arg.recv = recv;
	arg.mid = mid;
	arg.argc = argc;
	arg.argv = argv;
	return rb_protect(protect_funcall0, (VALUE) &arg, state);
}

static	void
PutApplication(
	ProcessNode	*node)
{
	char *buf;
	int i;

ENTER_FUNC;
	rb_funcall(node_info,rb_intern("clear"),0);
	if		(  node->mcprec  !=  NULL  ) {
		buf = xmalloc(JSON_SizeValue(RubyConv,node->mcprec->value));
		JSON_PackValue(RubyConv,buf,node->mcprec->value);
		rb_hash_aset(node_info,rb_str_new2("mcp"),rb_str_new2(buf));
		xfree(buf);
	}
	if		(  node->linkrec  !=  NULL  ) {
		buf = xmalloc(JSON_SizeValue(RubyConv,node->linkrec->value));
		JSON_PackValue(RubyConv,buf,node->linkrec->value);
		rb_hash_aset(node_info,rb_str_new2("link"),rb_str_new2(buf));
		xfree(buf);
	}
	if		(  node->sparec  !=  NULL  ) {
		buf = xmalloc(JSON_SizeValue(RubyConv,node->sparec->value));
		JSON_PackValue(RubyConv,buf,node->sparec->value);
		rb_hash_aset(node_info,rb_str_new2("spa"),rb_str_new2(buf));
		xfree(buf);
	}
	for	( i = 0; i < node->cWindow ; i ++ ) {
		if		(  node->scrrec[i]  !=  NULL  ) {
			buf = xmalloc(JSON_SizeValue(RubyConv,node->scrrec[i]->value));
			JSON_PackValue(RubyConv,buf,node->scrrec[i]->value);
			rb_hash_aset(node_info,rb_str_new2(node->scrrec[i]->name),rb_str_new2(buf));
			xfree(buf);
		}
	}
LEAVE_FUNC;
}


static	void
GetApplication(
	ProcessNode	*node)
{
	int i;
	VALUE obj;
	
ENTER_FUNC;
	if		(  node->mcprec  !=  NULL  ) {
		obj = rb_hash_aref(node_info,rb_str_new2("mcp"));
		if (!NIL_P(obj)) {
			JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->mcprec->value);
		}
	}
	if		(  node->linkrec  !=  NULL  ) {
		obj = rb_hash_aref(node_info,rb_str_new2("link"));
		if (!NIL_P(obj)) {
			JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->linkrec->value);
		}
	}
	if		(  node->sparec  !=  NULL  ) {
		obj = rb_hash_aref(node_info,rb_str_new2("spa"));
		if (!NIL_P(obj)) {
			JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->sparec->value);
		}
	}
	for	( i = 0; i < node->cWindow ; i ++ ) {
		if		(  node->scrrec[i]  !=  NULL  ) {
			obj = rb_hash_aref(node_info,rb_str_new2(node->scrrec[i]->name));
			if (!NIL_P(obj)) {
				JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->scrrec[i]->value);
			}
		}
	}
LEAVE_FUNC;
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
		Warning("unexpected return");
		break;
	case TAG_NEXT:
		Warning("unexpected next");
		break;
	case TAG_BREAK:
		Warning("unexpected break");
		break;
	case TAG_REDO:
		Warning("unexpected redo");
		break;
	case TAG_RETRY:
		Warning("retry outside of rescue clause");
		break;
	case TAG_THROW:
		Warning("unexpected throw");
		break;
	case TAG_RAISE:
		Warning("unexpected raise");
		break;
	case TAG_FATAL:
		Warning("unexpected fatal");
		break;
	default:
		Warning("Unknown longjmp status %d", ex);
		break;
	}
	rb_backtrace();
	return status;
}

static VALUE
get_source_filename(char *class_name, char *path)
{
	VALUE filename;

	if (!isupper(*class_name)) {
		return Qnil;
	}

	filename = rb_str_new2(path);
	rb_str_cat(filename, "/", 1);
	rb_str_cat2(filename, class_name);
	rb_str_cat(filename, ".rb", 3);
	return filename;
}

static VALUE
load_application(char *path, char *name)
{
	VALUE app_class, class_name;
	VALUE filename;
	int state;

	class_name = rb_str_new2(name);
	rb_gc_register_address(&class_name);
	app_class = rb_hash_aref(application_classes, class_name);
	if (NIL_P(app_class)) {
		filename = get_source_filename(name, path);
		if (NIL_P(filename)) {
			Warning("invalid module name: %s\n", name);
			return Qnil;
		}
		
		rb_load_protect(filename, 0, &state);
		if (state && error_handle(state))
			return Qnil;
		app_class = rb_eval_string_protect(name, &state);
		rb_gc_register_address(&app_class);
        
		if (state && error_handle(state))
			return Qnil;
		rb_hash_aset(application_classes, class_name, app_class);
	}
	return app_class;
}


static	Bool
_ExecuteProcess(
	MessageHandler	*handler,
	ProcessNode	*node)
{
	VALUE app_class;
	VALUE app;
	ValueStruct *dc_module_value;
	char *dc_module;
	int state;
ENTER_FUNC;
	if (handler->loadpath == NULL) {
		Warning("loadpath is required");
		return FALSE;
	}
	dc_module_value = GetItemLongName(node->mcprec->value, "dc.module");
	dc_module = ValueStringPointer(dc_module_value);
	app_class = load_application(handler->loadpath, dc_module);
	if (NIL_P(app_class)) {
		Warning("%s is not found", dc_module);
		return FALSE;
	}
	app = rb_protect_funcall(app_class, rb_intern("new"), &state, 0);
	if (state && error_handle(state)) {
		return FALSE;
	}
	PutApplication(node);
	rb_protect_funcall(app,rb_intern("exec_process"),&state,1,node_info);
	if (state && error_handle(state)) {
		Warning("eval exec_process failed");
		return FALSE;
	}
	GetApplication(node);
	if (ValueInteger(GetItemLongName(node->mcprec->value, "rc")) < 0) {
		return FALSE;
	} else {
		return TRUE;
	}
LEAVE_FUNC;
}

static	void
init(
	MessageHandler	*handler)
{
	VALUE load_path;


	InitMONFUNC(RubyConv,
		JSON_PackValue,
		JSON_UnPackValue,
		JSON_SizeValue);

	ruby_init();
	ruby_init_loadpath();

	load_path = rb_eval_string("$LOAD_PATH");

	if (handler->loadpath == NULL) {
		handler->loadpath = getenv("APS_RUBY_PATH");
	}
	if (handler->loadpath == NULL) {
		handler->loadpath = getenv("RUBYLIB");
	}
	if (handler->loadpath == NULL) {
		handler->loadpath = MONTSUQI_LOAD_PATH;
	}
	rb_ary_push(load_path, rb_str_new2(handler->loadpath));

	application_classes = rb_hash_new();
	rb_gc_register_address(&application_classes);
	rb_global_variable(&application_classes);

	node_info = rb_hash_new();
	rb_gc_register_address(&node_info);
	rb_global_variable(&node_info);

	rb_define_global_function("monfunc", monfunc, 2);
	rb_define_global_const("MCP_OK"        ,INT2NUM(MCP_OK));
	rb_define_global_const("MCP_EOF"       ,INT2NUM(MCP_EOF));
	rb_define_global_const("MCP_NONFATAL"  ,INT2NUM(MCP_NONFATAL));
	rb_define_global_const("MCP_BAD_ARG"   ,INT2NUM(MCP_BAD_ARG));
	rb_define_global_const("MCP_BAD_FUNC"  ,INT2NUM(MCP_BAD_FUNC));
	rb_define_global_const("MCP_BAD_OTHER" ,INT2NUM(MCP_BAD_OTHER));
	rb_define_global_const("MCP_BAD_CONN"  ,INT2NUM(MCP_BAD_CONN));
}

static	void
_ReadyDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
	RubyConv = NewConvOpt();
	ConvSetSize(RubyConv,ThisLD->textsize,ThisLD->arraysize);
	ConvSetCodeset(RubyConv,ConvCodeset(handler->conv));
	init(handler);
LEAVE_FUNC;
}

static	void
_StopDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
_StopDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
_ReadyDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}


static	int
_StartBatch(
	MessageHandler	*handler,
	char	*name,
	char	*param)
{
	VALUE app_class;
	VALUE app;
	VALUE rc;
	int state;

ENTER_FUNC;
#ifdef	DEBUG
	printf("starting [%s][%s]\n",name,param);
#endif
	RubyConv = NewConvOpt();
	ConvSetSize(RubyConv,ThisBD->textsize,ThisBD->arraysize);
	ConvSetCodeset(RubyConv,ConvCodeset(handler->conv));
	init(handler);

	app_class = load_application(handler->loadpath,name);
	if (NIL_P(app_class)) {
		Warning("%s in not found.",name);
		return -1;
	}
	app = rb_protect_funcall(app_class,rb_intern("new"),&state,0);
	if (state && error_handle(state)) {
		return -1;
	}
	rc = rb_protect_funcall(app, rb_intern("start_batch"), &state,
		1, rb_str_new2(param));
	if (state && error_handle(state))
		Warning("eval start_batch failed");
		return -1;
	if (FIXNUM_P(rc)) {
		return NUM2INT(rc);
	} else {
		return -1;
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
_ReadyExecute(
	MessageHandler	*handler,
	char	*loadpath)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	MessageHandlerClass	Handler = {
	"Ruby",
	_ReadyExecute,
	_ExecuteProcess,
	_StartBatch,
	_ReadyDC,
	_StopDC,
	NULL,
	_ReadyDB,
	_StopDB,
	NULL
};

extern	MessageHandlerClass	*
Ruby(void)
{
ENTER_FUNC;
LEAVE_FUNC;
	return	(&Handler);
}
#endif
