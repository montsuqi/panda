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

#include	"const.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	"types.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"mhandler.h"
#include	"Dlex.h"
#include	"directory.h"
#include	"debug.h"

static	GHashTable	*Handler;

static	MessageHandler	*
NewMessageHandler(
	char	*name,
	char	*klass)
{
	MessageHandler	*handler;

	handler = New(MessageHandler);
	handler->name = StrDup(name);
	handler->klass = (MessageHandlerClass *)klass;
	handler->serialize = NULL;
	handler->conv = New(CONVOPT);
	handler->conv->encode = STRING_ENCODING_URL;
	handler->start = NULL;
	handler->fInit = 0;
	handler->loadpath = NULL;
	handler->private = NULL;
	g_hash_table_insert(Handler,handler->name,handler);

	return	(handler);
}

extern	void
ParHANDLER(
	CURFILE	*in)
{
	MessageHandler	*handler;

ENTER_FUNC;
	GetSymbol; 
	if		(	(  ComToken  ==  T_SYMBOL  )
			||	(  ComToken  ==  T_SCONST  ) ) {
		if		(  ( handler = (MessageHandler *)g_hash_table_lookup(Handler,ComSymbol) )
				   ==  NULL  ) {
			handler = NewMessageHandler(ComSymbol,NULL);
		}
		if		(  GetSymbol  ==  '{'  ) {
			while	(  GetSymbol  !=  '}'  ) {
				switch	(ComToken) {
				  case	T_CLASS:
					if		(  GetName   ==  T_SCONST  ) {
						handler->klass = (MessageHandlerClass *)StrDup(ComSymbol);
					} else {
						Error("class must be string.");
					}
					break;
				  case	T_SERIALIZE:
					if		(  GetName   ==  T_SCONST  ) {
						handler->serialize = (ConvFuncs *)StrDup(ComSymbol);
					} else {
						Error("serialize method must be string.");
					}
					break;
				  case	T_START:
					if		(  GetName   ==  T_SCONST  ) {
						handler->start = StrDup(ComSymbol);
					} else {
						Error("start parameter must be string.");
					}
					break;
				  case	T_LOCALE:
					if		(  GetName   ==  T_SCONST  ) {
						handler->conv->codeset = StrDup(ComSymbol);
					} else {
						Error("locale name must be string.");
					}
					break;
				  case	T_LOADPATH:
					if		(  GetName   ==  T_SCONST  ) {
						handler->loadpath =
                            StrDup(ExpandPath(ComSymbol,ThisEnv->BaseDir));
					} else {
						Error("load path must be string.");
					}
					break;
				  case	T_ENCODING:
					if		(  GetName   ==  T_SCONST  ) {
						if		(  !stricmp(ComSymbol,"URL")  ) {
							handler->conv->encode = STRING_ENCODING_URL;
						} else
						if		(  !stricmp(ComSymbol,"BASE64")  ) {
							handler->conv->encode = STRING_ENCODING_BASE64;
						} else {
							Error("unsupported string encoding");
						}
					} else {
						Error("string encoding must be string.");
					}
					break;
				  default:
					Error("%s:%d:handler parameter(s): %s",
                          in->fn, in->cLine, ComSymbol);
					break;
				}
				if		(  GetSymbol  !=  ';'  ) {
					Error("parameter ; missing");
				}
			}
		} else {
			Error("invalid char");
		}
	} else {
		Error("invalid handler name");
	}
LEAVE_FUNC;
}

extern	void
BindMessageHandlerCommon(
	MessageHandler	**handler)
{
	MessageHandler	*h;

	if		(  ( h = (MessageHandler *)g_hash_table_lookup(Handler,*(char **)handler) )
			   !=  NULL  ) {
		*handler = h;
	} else {
		Error("invalid handler name: %s", (*handler)->name);
	}
}

static	void
EnterDefaultHandler(void)
{
	MessageHandler	*handler;

#ifdef	HAVE_OPENCOBOL
	handler = NewMessageHandler("OpenCOBOL","OpenCOBOL");
	handler->serialize = (ConvFuncs *)"OpenCOBOL";
	ConvSetCodeset(handler->conv,"euc-jp");
	handler->start = "";
#endif
	handler = NewMessageHandler("C","C");
	handler->serialize = NULL;
	ConvSetCodeset(handler->conv,NULL);
	handler->start = "";

	handler = NewMessageHandler("Exec","Exec");
	handler->serialize = (ConvFuncs *)"CGI";
	ConvSetCodeset(handler->conv,"euc-jp");
	handler->start = "%m";
}

extern	void
MessageHandlerInit(void)
{
	Handler = NewNameHash();
	EnterDefaultHandler();
}
