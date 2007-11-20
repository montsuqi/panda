/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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
 *
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

ENTER_FUNC;
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
LEAVE_FUNC;
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

ENTER_FUNC;
	if		(  ( h = (MessageHandler *)g_hash_table_lookup(Handler,*(char **)handler) )
			   !=  NULL  ) {
		*handler = h;
	} else {
		Error("invalid handler name: %s",*(char **)handler);
	}
LEAVE_FUNC;
}

static	void
EnterDefaultHandler(void)
{
	MessageHandler	*handler;

ENTER_FUNC;
#ifdef	HAVE_OPENCOBOL
	handler = NewMessageHandler("OpenCOBOL","OpenCOBOL");
	handler->serialize = (ConvFuncs *)"OpenCOBOL";
	ConvSetCodeset(handler->conv,"euc-jp");
	handler->start = "";
#ifdef HAVE_OPENCOBOL23
	handler = NewMessageHandler("OpenCOBOL23","OpenCOBOL23");
	handler->serialize = (ConvFuncs *)"OpenCOBOL23";
 	ConvSetCodeset(handler->conv,"euc-jp");
	handler->start = "";
#endif
#ifdef HAVE_OPENCOBOL08
	handler = NewMessageHandler("OpenCOBOL08","OpenCOBOL08");
	handler->serialize = (ConvFuncs *)"OpenCOBOL08";
	ConvSetCodeset(handler->conv,"euc-jp");
	handler->start = "";
#endif
#endif
	handler = NewMessageHandler("C","C");
	handler->serialize = NULL;
	ConvSetCodeset(handler->conv,NULL);
	handler->start = "";

	handler = NewMessageHandler("Exec","Exec");
	handler->serialize = (ConvFuncs *)"CGI";
	ConvSetCodeset(handler->conv,"euc-jp");
	handler->start = "%m";
LEAVE_FUNC;
}

extern	void
MessageHandlerInit(void)
{
ENTER_FUNC;
	Handler = NewNameHash();
	EnterDefaultHandler();
LEAVE_FUNC;
}
