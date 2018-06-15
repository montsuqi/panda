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
#include "DBparser.h"
#include "DBDparser.h"
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

                                  {"", 0}};

static GHashTable *Reserved;

static void DBD_Par(CURFILE *in, DBD_Struct *dbd, char *gname) {
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
            if (dbd != NULL && g_hash_table_lookup(dbd->DBD_Table, ComSymbol) == NULL) {
              rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) *
                                              (dbd->cDB + 1));
              memcpy(rtmp, dbd->db, sizeof(RecordStruct *) * dbd->cDB);
              xfree(dbd->db);
              dbd->db = rtmp;
              dbd->db[dbd->cDB] = db;
              dbd->cDB++;
              g_hash_table_insert(dbd->DBD_Table, StrDup(ComSymbol),
                                  (void *)dbd->cDB);
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

static DBD_Struct *NewDBD(void) {
  DBD_Struct *dbd;
  ENTER_FUNC;
  dbd = New(DBD_Struct);
  dbd->name = NULL;
  dbd->cDB = 1;
  dbd->db = (RecordStruct **)xmalloc(sizeof(RecordStruct *));
  dbd->db[0] = NULL;
  dbd->arraysize = SIZE_DEFAULT_ARRAY_SIZE;
  dbd->textsize = SIZE_DEFAULT_TEXT_SIZE;
  dbd->DBD_Table = NewNameHash();
  LEAVE_FUNC;
  return (dbd);
}

static DBD_Struct *ParDB(CURFILE *in) {
  DBD_Struct *ret;
  char *gname;

  ENTER_FUNC;
  ret = NULL;
  while (GetSymbol != T_EOF) {
    switch (ComToken) {
    case T_NAME:
      if (GetName != T_SYMBOL) {
        ParError("no name");
      } else {
        ret = NewDBD();
        ret->name = StrDup(ComSymbol);
      }
      break;
    case T_ARRAYSIZE:
      if (GetSymbol == T_ICONST && ret != NULL) {
        ret->arraysize = ComInt;
      } else {
        ParError("invalid array size");
      }
      break;
    case T_TEXTSIZE:
      if (GetSymbol == T_ICONST && ret != NULL) {
        ret->textsize = ComInt;
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
      DBD_Par(in, ret, gname);
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

extern DBD_Struct *DBD_Parser(char *name) {
  DBD_Struct *ret;
  struct stat stbuf;
  CURFILE *in, root;

  ENTER_FUNC;
  root.next = NULL;
  if (stat(name, &stbuf) == 0) {
    if ((in = PushLexInfo(&root, name, D_Dir, Reserved)) != NULL) {
      ret = ParDB(in);
      DropLexInfo(&in);
    } else {
      if (in != NULL) {
        ParError("DBD file not found");
      }
      ret = NULL;
    }
  } else {
    ret = NULL;
  }
  LEAVE_FUNC;
  return (ret);
}

extern void DBD_ParserInit(void) {
  LexInit();
  Reserved = MakeReservedTable(tokentable);

  DBD_Table = NewNameHash();
}
