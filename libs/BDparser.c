/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
#define	MAIN
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _D_PARSER

#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "libmondai.h"
#include "dbgroup.h"
#include "mhandler.h"
#include "DBparser.h"
#include "BDparser.h"
#include "directory.h"
#include "Dlex.h"
#include "dirs.h"
#include "debug.h"

static TokenTable tokentable[] = {{"data", T_DATA},
                                  {"host", T_HOST},
                                  {"name", T_NAME},
                                  {"home", T_HOME},
                                  {"port", T_PORT},
                                  {"spa", T_SPA},
                                  {"window", T_WINDOW},
                                  {"cache", T_CACHE},
                                  {"arraysize", T_ARRAYSIZE},
                                  {"textsize", T_TEXTSIZE},
                                  {"db", T_DB},
                                  {"multiplex_group", T_MGROUP},
                                  {"bind", T_BIND},

                                  {"handler", T_HANDLER},
                                  {"class", T_CLASS},
                                  {"serialize", T_SERIALIZE},
                                  {"start", T_START},
                                  {"locale", T_LOCALE},
                                  {"encoding", T_ENCODING},
                                  {"handlerpath", T_HANDLERPATH},
                                  {"loadpath", T_LOADPATH},
                                  {"bigendian", T_BIGENDIAN},

                                  {"", 0}};

static GHashTable *Reserved;

static void ParDB(CURFILE *in, BD_Struct *bd, char *gname) {
  RecordStruct *db;
  RecordStruct **rtmp;
  char name[SIZE_BUFF];
  char buff[SIZE_BUFF];
  char *p, *q;

  ENTER_FUNC;
  while (GetSymbol != '}') {
    if ((ComToken == T_SYMBOL) || (ComToken == T_SCONST)) {
      if (stricmp(ComSymbol, "metadb")) {
        strcpy(buff, RecordDir);
        p = buff;
        do {
          if ((q = strchr(p, ':')) != NULL) {
            *q = 0;
          }
          sprintf(name, "%s/%s.db", p, ComSymbol);
          if ((db = DB_Parser(name, gname, TRUE)) != NULL) {
            if (bd == NULL) {
              Error("bd is NULL");
            } else if (g_hash_table_lookup(bd->DB_Table, ComSymbol) == NULL) {
              rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) *
                                              (bd->cDB + 1));
              memcpy(rtmp, bd->db, sizeof(RecordStruct *) * bd->cDB);
              xfree(bd->db);
              bd->db = rtmp;
              bd->db[bd->cDB] = db;
              bd->cDB++;
              g_hash_table_insert(bd->DB_Table, StrDup(ComSymbol),
                                  (void *)bd->cDB);
            } else {
              ParError("same db appier");
            }
          }
          p = q + 1;
        } while ((q != NULL) && (db == NULL));
        if (db == NULL) {
          ParError("db not found");
        }
      }
    }
    if (GetSymbol != ';') {
      ParError("DB ; missing");
    }
    ERROR_BREAK;
  }
  xfree(gname);
  LEAVE_FUNC;
}

static void ParBIND(CURFILE *in, BD_Struct *ret) {
  BatchBind *bind;

  ENTER_FUNC;
  if ((GetSymbol == T_SCONST) || (ComToken == T_SYMBOL)) {
    if (ret == NULL) {
      Error("ret is NULL");
    } else if ((bind = g_hash_table_lookup(ret->BatchTable, ComSymbol)) == NULL) {
      bind = New(BatchBind);
      bind->module = StrDup(ComSymbol);
      ;
      g_hash_table_insert(ret->BatchTable, bind->module, bind);
    }
    if ((GetSymbol == T_SCONST) || (ComToken == T_SYMBOL)) {
      if (bind != NULL) {
        bind->handler = (void *)StrDup(ComSymbol);
      } else {
        Error("bind is NULL");
      }
    } else {
      ParError("handler name error");
    }
  } else {
    ParError("module name error");
  }
  LEAVE_FUNC;
}

static BD_Struct *NewBD(void) {
  BD_Struct *bd;
  ENTER_FUNC;
  bd = New(BD_Struct);
  bd->name = NULL;
  bd->cDB = 1;
  bd->db = (RecordStruct **)xmalloc(sizeof(RecordStruct *));
  bd->db[0] = NULL;
  bd->arraysize = SIZE_DEFAULT_ARRAY_SIZE;
  bd->textsize = SIZE_DEFAULT_TEXT_SIZE;
  bd->DB_Table = NewNameHash();
  bd->BatchTable = NewNameHash();
  bd->home = NULL;
  bd->loadpath = NULL;
  bd->handlerpath = NULL;
  LEAVE_FUNC;
  return (bd);
}

static BD_Struct *BD_Par(CURFILE *in) {
  BD_Struct *ret;
  char *gname;

  ENTER_FUNC;
  ret = NULL;
  while (GetSymbol != T_EOF) {
    switch (ComToken) {
    case T_NAME:
      if (GetName != T_SYMBOL) {
        ParError("no name");
      } else {
        ret = NewBD();
        ret->name = StrDup(ComSymbol);
      }
      break;
    case T_ARRAYSIZE:
      if (GetSymbol == T_ICONST) {
        if (ret != NULL) {
          ret->arraysize = ComInt;
        } else {
          Error("ret is NULL");
        }
      } else {
        ParError("invalid array size");
      }
      break;
    case T_TEXTSIZE:
      if (GetSymbol == T_ICONST) {
        if (ret != NULL) {
          ret->textsize = ComInt;
        } else {
          Error("ret is NULL");
        }
      } else {
        ParError("invalid text size");
      }
      break;
    case T_DB:
      if (GetSymbol == T_SCONST) {
        gname = StrDup(ComSymbol);
        if (GetSymbol != '{') {
          ParError("syntax error 3");
        }
      } else if (ComToken == '{') {
        gname = StrDup("");
      } else {
        gname = NULL;
        ParError("syntax error 4");
      }
      ParDB(in, ret, gname);
      break;
    case T_HOME:
      if (GetSymbol == T_SCONST) {
        if (ret != NULL) {
          ret->home = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          Error("ret is NULL");
        }
      } else {
        ParError("home directory invalid");
      }
      break;
    case T_LOADPATH:
      if (GetSymbol == T_SCONST) {
        if (ret != NULL) {
          ret->loadpath = strdup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ERROR("ret is NULL");
        }
      } else {
        ParError("load path invalid");
      }
      break;
    case T_HANDLERPATH:
      if (GetSymbol == T_SCONST) {
        if (ret != NULL) {
          ret->handlerpath = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ERROR("ret is null");
        }
      } else {
        ParError("handler path invalid");
      }
      break;
    case T_BIND:
      ParBIND(in, ret);
      break;
    case T_HANDLER:
      ParHANDLER(in);
      break;
    default:
      ParError("syntax error 3");
      break;
    }
    if (GetSymbol != ';') {
      ParError("; missing");
    }
    ERROR_BREAK;
  }
  LEAVE_FUNC;
  return (ret);
}

static void _BindHandler(char *name, BatchBind *bind, void *dummy) {
  BindMessageHandlerCommon(&bind->handler);
}

static void BindHandler(BD_Struct *bd) {
  g_hash_table_foreach(bd->BatchTable, (GHFunc)_BindHandler, NULL);
}

extern BD_Struct *BD_Parser(char *name) {
  BD_Struct *ret;
  struct stat stbuf;
  CURFILE *in, root;

  ENTER_FUNC;
  root.next = NULL;
  if (stat(name, &stbuf) == 0) {
    if ((in = PushLexInfo(&root, name, ThisEnv->D_Dir, Reserved)) != NULL) {
      ret = BD_Par(in);
      DropLexInfo(&in);
      BindHandler(ret);
    } else {
      if (in != NULL) {
        ParErrorPrintf("BD file not found %s", name);
      } else {
        Error("in is NULL");
      }
      ret = NULL;
    }
  } else {
    ret = NULL;
  }
  LEAVE_FUNC;
  return (ret);
}

extern void BD_ParserInit(void) {
  LexInit();
  Reserved = MakeReservedTable(tokentable);

  BD_Table = NewNameHash();
  MessageHandlerInit();
}
