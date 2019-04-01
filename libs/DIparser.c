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
 *
 */

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _DI_PARSER
#define DIRS_MAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <sys/stat.h> /*	for stbuf	*/
#include <unistd.h>
#include <libgen.h>
#include "const.h"
#include "dirs.h"
#include <libmondai.h>
#include <RecParser.h>
#include "DDparser.h"
#include "Lex.h"
#include "LDparser.h"
#include "BDparser.h"
#include "DBDparser.h"
#include "DIparser.h"
#define _DIRECTORY
#include "directory.h"
#include "debug.h"

#define T_NAME (T_YYBASE + 1)
#define T_LD (T_YYBASE + 2)
#define T_LINKSIZE (T_YYBASE + 3)
#define T_MULTI (T_YYBASE + 4)
#define T_BD (T_YYBASE + 5)
#define T_DBGROUP (T_YYBASE + 6)
#define T_PORT (T_YYBASE + 7)
#define T_USER (T_YYBASE + 8)
#define T_PASS (T_YYBASE + 9)
#define T_TYPE (T_YYBASE + 10)
#define T_FILE (T_YYBASE + 11)
#define T_REDIRECT (T_YYBASE + 12)
#define T_REDIRECTPORT (T_YYBASE + 13)
#define T_RECDIR (T_YYBASE + 16)
#define T_BASEDIR (T_YYBASE + 17)
#define T_PRIORITY (T_YYBASE + 18)
#define T_LINKAGE (T_YYBASE + 19)
#define T_STACKSIZE (T_YYBASE + 20)
#define T_DB (T_YYBASE + 21)
#define T_WFC (T_YYBASE + 23)
#define T_EXIT (T_YYBASE + 24)
#define T_ENCODING (T_YYBASE + 25)
#define T_TERMPORT (T_YYBASE + 26)
#define T_CONTROL (T_YYBASE + 27)
#define T_DDIR (T_YYBASE + 28)
#define T_SYSDATA (T_YYBASE + 29)
#define T_AUTH (T_YYBASE + 30)
#define T_SPACE (T_YYBASE + 31)
#define T_APSPATH (T_YYBASE + 32)
#define T_WFCPATH (T_YYBASE + 33)
#define T_REDPATH (T_YYBASE + 34)
#define T_DBPATH (T_YYBASE + 35)
#define T_UPDATE (T_YYBASE + 36)   /*deprecated*/
#define T_READONLY (T_YYBASE + 37) /*deprecated*/
#define T_SSLMODE (T_YYBASE + 38)
#define T_SSLCERT (T_YYBASE + 39)
#define T_SSLKEY (T_YYBASE + 40)
#define T_SSLROOTCERT (T_YYBASE + 41)
#define T_SSLCRL (T_YYBASE + 42)
#define T_BLOB (T_YYBASE + 43)

#define T_LOGDBNAME (T_YYBASE + 44)
#define T_LOGTABLENAME (T_YYBASE + 45)
#define T_LOGPORT (T_YYBASE + 46)
#define T_LOGPATH (T_YYBASE + 47)
#define T_MSTPATH (T_YYBASE + 48)
#define T_SLVPATH (T_YYBASE + 49)
#define T_MSTPORT (T_YYBASE + 50)
#define T_DBMASTER (T_YYBASE + 51)
#define T_AUDITLOG (T_YYBASE + 52)
#define T_SUMCHECK (T_YYBASE + 53)

#define T_INITIAL_LD (T_YYBASE + 54)
#define T_CRYPTOPASS (T_YYBASE + 55)

static TokenTable tokentable[] = {{"ld", T_LD},
                                  {"initial_ld", T_INITIAL_LD},
                                  {"bd", T_BD},
                                  {"db", T_DB},
                                  {"name", T_NAME},
                                  {"linksize", T_LINKSIZE},
                                  {"multiplex_level", T_MULTI},
                                  {"db_group", T_DBGROUP},
                                  {"port", T_PORT},
                                  {"user", T_USER},
                                  {"password", T_PASS},
                                  {"sslmode", T_SSLMODE},
                                  {"sslcert", T_SSLCERT},
                                  {"sslkey", T_SSLKEY},
                                  {"sslrootcert", T_SSLROOTCERT},
                                  {"sslcrl", T_SSLCRL},
                                  {"type", T_TYPE},
                                  {"file", T_FILE},
                                  {"redirect", T_REDIRECT},
                                  {"redirect_port", T_REDIRECTPORT},
                                  {"crypto_password", T_CRYPTOPASS},
                                  {"ddir", T_DDIR},
                                  {"record", T_RECDIR},
                                  {"base", T_BASEDIR},
                                  {"priority", T_PRIORITY},
                                  {"linkage", T_LINKAGE},
                                  {"stacksize", T_STACKSIZE},
                                  {"wfc", T_WFC},
                                  {"exit", T_EXIT},
                                  {"locale", T_ENCODING},
                                  {"encoding", T_ENCODING},
                                  {"termport", T_TERMPORT},
                                  {"control", T_CONTROL},
                                  {"sysdata", T_SYSDATA},
                                  {"auth", T_AUTH},
                                  {"space", T_SPACE},
                                  {"apspath", T_APSPATH},
                                  {"wfcpath", T_WFCPATH},
                                  {"redpath", T_REDPATH},
                                  {"dbpath", T_DBPATH},
                                  {"update", T_UPDATE},
                                  {"readonly", T_READONLY},
                                  {"blob", T_BLOB},
                                  {"logdb_name", T_LOGDBNAME},
                                  {"logtable_name", T_LOGTABLENAME},
                                  {"logport", T_LOGPORT},
                                  {"logpath", T_LOGPATH},
                                  {"mstpath", T_MSTPATH},
                                  {"slvpath", T_SLVPATH},
                                  {"mstport", T_MSTPORT},
                                  {"dbmaster", T_DBMASTER},
                                  {"auditlog", T_AUDITLOG},
                                  {"sumcheck", T_SUMCHECK},
                                  {"", 0}};

static GHashTable *Reserved;

static void ParWFC(CURFILE *in) {
  while (GetSymbol != '}') {
    switch (ComToken) {
    case T_PORT:
      switch (GetSymbol) {
      case T_ICONST:
        DestroyPort(ThisEnv->WfcApsPort);
        ThisEnv->WfcApsPort = NewIP_Port(NULL, IntStrDup(ComInt));
        break;
      case T_SCONST:
        DestroyPort(ThisEnv->WfcApsPort);
        ThisEnv->WfcApsPort = ParPort(ComSymbol, PORT_WFC_APS);
        break;
      default:
        ParError("invalid port number");
        break;
      }
      GetSymbol;
      break;
    case T_TERMPORT:
      switch (GetSymbol) {
      case T_ICONST:
        DestroyPort(ThisEnv->TermPort);
        ThisEnv->TermPort = NewIP_Port(NULL, IntStrDup(ComInt));
        break;
      case T_SCONST:
        DestroyPort(ThisEnv->TermPort);
        ThisEnv->TermPort = ParPort(ComSymbol, PORT_WFC);
        break;
      default:
        ParError("invalid port number");
        break;
      }
      GetSymbol;
      break;
    default:
      ParError("wfc keyword error");
      break;
    }
    if (ComToken != ';') {
      ParError("missing ; in wfc directive");
    }
    ERROR_BREAK;
  }
}

static void ParCONTROL(CURFILE *in) {
  while (GetSymbol != '}') {
    switch (ComToken) {
    case T_PORT:
      switch (GetSymbol) {
      case T_ICONST:
        DestroyPort(ThisEnv->ControlPort);
        ThisEnv->ControlPort = NewIP_Port(NULL, IntStrDup(ComInt));
        GetSymbol;
        break;
      case T_SCONST:
        DestroyPort(ThisEnv->ControlPort);
        ThisEnv->ControlPort = ParPort(ComSymbol, PORT_WFC_CONTROL);
        GetSymbol;
        break;
      case ';':
        ThisEnv->ControlPort = NULL;
        break;
      default:
        ParError("invalid port number");
        break;
      }
      break;
    default:
      ParError("control keyword error");
      break;
    }
    if (ComToken != ';') {
      ParError("missing ; in control directive");
    }
    ERROR_BREAK;
  }
}

static void ParDBMaster(CURFILE *in) {
  while (GetSymbol != '}') {
    switch (ComToken) {
    case T_PORT:
      switch (GetSymbol) {
      case T_ICONST:
        DestroyPort(ThisEnv->DBMasterPort);
        ThisEnv->DBMasterPort = NewIP_Port(NULL, IntStrDup(ComInt));
        GetSymbol;
        break;
      case T_SCONST:
        DestroyPort(ThisEnv->DBMasterPort);
        ThisEnv->DBMasterPort = ParPort(ComSymbol, PORT_WFC_CONTROL);
        GetSymbol;
        break;
      case ';':
        ThisEnv->DBMasterPort = NULL;
        break;
      default:
        ParError("invalid dbmaster port number");
        break;
      }
      break;
    case T_LOGDBNAME:
      if (GetSymbol == T_SCONST) {
        ThisEnv->DBMasterLogDBName = StrDup(ComSymbol);
      } else {
        ParError("invalid dbmaster group");
      }
      GetSymbol;
      break;
    case T_AUTH:
      if (GetSymbol == T_SCONST) {
        ThisEnv->DBMasterAuth = StrDup(ComSymbol);
      } else {
        ParError("invalid dbmaster group");
      }
      GetSymbol;
      break;
    default:
      ParError("dbmaster keyword error");
      break;
    }
    if (ComToken != ';') {
      ParError("missing ; in dbmaster directive");
    }
    ERROR_BREAK;
  }
  if (ThisEnv->DBMasterPort == NULL) {
    ParError(" port is null");
  }

}

extern LD_Struct *LD_DummyParser(CURFILE *in) {
  LD_Struct *ret;

  ret = New(LD_Struct);
  ret->name = StrDup(ComSymbol);
  return (ret);
}

static SysData_Struct *ParSysData(CURFILE *in) {
  SysData_Struct *sysdata;

  sysdata = New(SysData_Struct);
  sysdata->port = NULL;
  sysdata->dir = NULL;
  while (GetSymbol != '}') {
    switch (ComToken) {
    case T_PORT:
      switch (GetSymbol) {
      case T_ICONST:
        sysdata->port = NewIP_Port(NULL, IntStrDup(ComInt));
        GetSymbol;
        break;
      case T_SCONST:
        sysdata->port = ParPort(ComSymbol, SYSDATA_PORT);
        GetSymbol;
        break;
      case ';':
        sysdata->port = NULL;
        break;
      default:
        ParError("invalid port number");
        break;
      }
      break;
    case T_SPACE:
      switch (GetSymbol) {
      case T_SCONST:
        sysdata->dir = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        GetSymbol;
        break;
      case ';':
        sysdata->dir = NULL;
        break;
      default:
        ParError("invalid sysdata space");
        break;
      }
      break;
    default:
      ParError("sysdata keyword error");
      break;
    }
    if (ComToken != ';') {
      ParError("missing ; in sysdata directive");
    }
    ERROR_BREAK;
  }
  return (sysdata);
}

static void ParLD_Elements(CURFILE *in, int parse_type) {
  char buff[SIZE_BUFF];
  char name[SIZE_BUFF];
  char *p, *q;
  LD_Struct **tmp;
  int i, n;
  Port **tports;
  LD_Struct *ld;

  strcpy(buff, ThisEnv->D_Dir);
  p = buff;
  do {
    if ((q = strchr(p, ':')) != NULL) {
      *q = 0;
    }
    sprintf(name, "%s/%s.ld", p, ComSymbol);
    dbgprintf("[%s]\n", name);
    if (parse_type >= P_LD) {
      ld = LD_Parser(name, parse_type);
    } else {
      ld = LD_DummyParser(in);
    }
    if (ld != NULL) {
      if (g_hash_table_lookup(ThisEnv->LD_Table, ComSymbol) != NULL) {
        ParError("same ld appier");
      }
      tmp = (LD_Struct **)xmalloc(sizeof(LD_Struct *) * (ThisEnv->cLD + 1));
      if (ThisEnv->cLD > 0) {
        memcpy(tmp, ThisEnv->ld, sizeof(LD_Struct *) * ThisEnv->cLD);
        xfree(ThisEnv->ld);
      }
      ThisEnv->ld = tmp;
      ThisEnv->ld[ThisEnv->cLD] = ld;
      ThisEnv->cLD++;
      g_hash_table_insert(ThisEnv->LD_Table, StrDup(ComSymbol), ld);
      switch (ComToken) {
      case T_SYMBOL:
      case T_SCONST:
        dbgprintf("[%s]", ComSymbol);
        dbgmsg("symbol");
        ld->nports = 0;
        while ((ComToken == T_SCONST) || (ComToken == T_SYMBOL)) {
          strcpy(buff, ComSymbol);
          n = 0;
          switch (GetSymbol) {
          case ',':
            n = 1;
            break;
          case '*':
            if (GetSymbol == T_ICONST) {
              n = ComInt;
              GetSymbol;
            } else {
              ParError("invalid ld multiplicity number");
            }
            break;
          case T_ICONST:
            n = ComInt;
            GetSymbol;
            break;
          case ';':
            n = 1;
            break;
          default:
            ParError("invalid operator(ld multiplicity)");
            break;
          }
          tports = (Port **)xmalloc(sizeof(Port *) * (ld->nports + n));
          if (ld->nports > 0) {
            memcpy(tports, ld->ports, sizeof(Port *) * ld->nports);
            xfree(ld->ports);
          }
          ld->ports = tports;
          for (i = 0; i < n; i++) {
            ld->ports[ld->nports] =
                ParPort(buff, PORT_WFC); // ldname in buff .what meaning?
            ld->nports++;
          }
          if (ComToken == ',') {
            GetSymbol;
          }
        }
        break;
      case T_ICONST:
        dbgmsg("iconst");
        ld->nports = ComInt;
        ld->ports = (Port **)xmalloc(sizeof(Port *) * ld->nports);
        for (i = 0; i < ld->nports; i++) {
          ld->ports[i] = NULL;
        }
        GetSymbol;
        break;
      default:
        dbgmsg("else");
        ld->ports = (Port **)xmalloc(sizeof(Port *));
        ld->ports[0] = ParPort("localhost", NULL);
        ld->nports = 1;
      }
    }
    p = q + 1;
    ERROR_BREAK;
  } while ((q != NULL) && (ld == NULL));
  if (ld == NULL) {
    ParErrorPrintf("ld file not found %s.ld\n", ComSymbol);
  }
}

static void SkipLD(CURFILE *in) {
  while (ComToken != ';') {
    switch (ComToken) {
    case T_SYMBOL:
    case T_SCONST:
      switch (GetSymbol) {
      case ',':
        break;
      case '*':
        if (GetSymbol == T_ICONST) {
          GetSymbol;
        } else {
          ParError("invalid number");
        }
        break;
      case T_ICONST:
        GetSymbol;
        break;
      }
      if (ComToken == ',') {
        GetSymbol;
      }
      break;
    case T_ICONST:
      GetSymbol;
      break;
    default:
      break;
    }
    ERROR_BREAK;
  }
}

static void ParLD(CURFILE *in, char *dname, int parse_type) {
  while (GetSymbol != '}') {
    if ((ComToken == T_SYMBOL) || (ComToken == T_SCONST) ||
        (ComToken == T_EXIT)) {
      if (ThisEnv->D_Dir == NULL) {
        SkipLD(in);
      } else {
        if ((dname == NULL) || (!strcmp(ComSymbol, dname))) {
          ParLD_Elements(in, parse_type);
        } else {
          SkipLD(in);
        }
      }
      if (ComToken != ';') {
        ParError("syntax error 2");
      }
    }
    ERROR_BREAK;
  }
}

static void ParBD(CURFILE *in, char *dname, int parse_type) {
  char name[SIZE_BUFF];
  BD_Struct *bd;
  BD_Struct **btmp;
  char buff[SIZE_BUFF];
  char *p, *q;

  bd = NULL;
  while (GetSymbol != '}') {
    if ((ComToken == T_SYMBOL) || (ComToken == T_SCONST)) {
      if (ThisEnv->D_Dir != NULL) {
        if ((dname == NULL) || (!strcmp(dname, ComSymbol))) {
          strcpy(buff, ThisEnv->D_Dir);
          p = buff;
          do {
            if ((q = strchr(p, ':')) != NULL) {
              *q = 0;
            }
            sprintf(name, "%s/%s.bd", p, ComSymbol);
            if (parse_type >= P_LD) {
              bd = BD_Parser(name);
              if (bd != NULL) {
                if (g_hash_table_lookup(ThisEnv->BD_Table, ComSymbol) == NULL) {
                  btmp = (BD_Struct **)xmalloc(sizeof(BD_Struct *) *
                                               (ThisEnv->cBD + 1));
                  if (ThisEnv->cBD > 0) {
                    memcpy(btmp, ThisEnv->bd,
                           sizeof(BD_Struct *) * ThisEnv->cBD);
                    xfree(ThisEnv->bd);
                  }
                  ThisEnv->bd = btmp;
                  ThisEnv->bd[ThisEnv->cBD] = bd;
                  ThisEnv->cBD++;
                  g_hash_table_insert(ThisEnv->BD_Table, StrDup(ComSymbol), bd);
                } else {
                  ParError("same bd appier");
                }
              }
              p = q + 1;
            }
          } while ((q != NULL) && (bd == NULL));
        }
      }
    }
    if (GetSymbol != ';') {
      ParError("syntax error 1");
    }
    ERROR_BREAK;
  }
}

static void ParDBD(CURFILE *in, char *dname) {
  char name[SIZE_BUFF];
  DBD_Struct *dbd;
  DBD_Struct **btmp;
  char buff[SIZE_BUFF];
  char *p, *q;

  while (GetSymbol != '}') {
    if ((ComToken == T_SYMBOL) || (ComToken == T_SCONST)) {
      if (ThisEnv->D_Dir != NULL) {
        if ((dname == NULL) || (!strcmp(dname, ComSymbol))) {
          strcpy(buff, ThisEnv->D_Dir);
          p = buff;
          do {
            if ((q = strchr(p, ':')) != NULL) {
              *q = 0;
            }
            sprintf(name, "%s/%s.dbd", p, ComSymbol);
            if ((dbd = DBD_Parser(name)) != NULL) {
              if (g_hash_table_lookup(ThisEnv->DBD_Table, ComSymbol) == NULL) {
                btmp = (DBD_Struct **)xmalloc(sizeof(DBD_Struct *) *
                                              (ThisEnv->cDBD + 1));
                if (ThisEnv->cDBD > 0) {
                  memcpy(btmp, ThisEnv->db,
                         sizeof(DBD_Struct *) * ThisEnv->cDBD);
                  xfree(ThisEnv->db);
                }
                ThisEnv->db = btmp;
                ThisEnv->db[ThisEnv->cDBD] = dbd;
                ThisEnv->cDBD++;
                g_hash_table_insert(ThisEnv->DBD_Table, StrDup(ComSymbol), dbd);
              } else {
                ParError("same db appier");
              }
            }
            p = q + 1;
          } while ((q != NULL) && (dbd == NULL));
          if (dbd == NULL) {
            ParError("dbd not found");
          }
        }
      }
    }
    if (GetSymbol != ';') {
      ParError("syntax error 1");
    }
    ERROR_BREAK;
  }
}

extern DBG_Struct *NewDBG_Struct(char *name) {
  DBG_Struct *dbg;
  char *env;

  dbg = New(DBG_Struct);
  dbg->name = StrDup(name);
  dbg->id = 0;
  dbg->type = NULL;
  dbg->func = NULL;
  dbg->transaction_id = NULL;
  dbg->file = NULL;
  dbg->sumcheck = 1;
  dbg->appname = NULL;
  dbg->redirect = NULL;
  dbg->redirectName = NULL;
  dbg->logTableName = NULL;
  dbg->redirectorMode = REDIRECTOR_MODE_PATCH;
  dbg->auditlog = 0;
  dbg->redirectPort = NULL;
  dbg->redirectData = NULL;
  dbg->checkData = NewLBS();
  dbg->last_query = NewLBS();
  dbg->fpLog = NULL;
  dbg->dbt = NULL;
  dbg->priority = 50;
  dbg->errcount = 0;
  dbg->dbstatus = DB_STATUS_NOCONNECT;
  dbg->conn = NULL;

  dbg->server = New(DB_Server);
  dbg->server->port = NULL;
  dbg->server->dbname = NULL;
  dbg->server->user = NULL;
  dbg->server->pass = NULL;
  dbg->server->sslmode = NULL;
  dbg->server->sslcert = NULL;
  dbg->server->sslkey = NULL;
  dbg->server->sslrootcert = NULL;
  dbg->server->sslcrl = NULL;

  if ((env = getenv("MONDB_LOCALE")) == NULL) {
    dbg->coding = DB_LOCALE;
  } else if (stricmp(env, "UTF8") == 0) {
    dbg->coding = NULL;
  }

  return (dbg);
}

extern void FreeDBG_Struct(DBG_Struct *dbg) {
  FreeLBS(dbg->checkData);
  FreeLBS(dbg->last_query);
  xfree(dbg->name);
  xfree(dbg);
}

static void ParDBGROUP(CURFILE *in, char *name) {
  DBG_Struct *dbg;

  if (g_hash_table_lookup(ThisEnv->DBG_Table, name) != NULL) {
    ParError("DB group name duplicate");
  }
  dbg = NewDBG_Struct(name);
  while (GetSymbol != '}') {
    switch (ComToken) {
    case T_TYPE:
      GetSymbol;
      if ((ComToken == T_SYMBOL) || (ComToken == T_SCONST)) {
        dbg->type = StrDup(ComSymbol);
      } else {
        ParError("invalid DBMS type");
      }
      break;
    case T_REDIRECTPORT:
      if (GetSymbol == T_SCONST) {
        dbg->redirectPort = ParPort(ComSymbol, PORT_REDIRECT);
      } else {
        ParError("invalid port");
      }
      break;
    case T_FILE:
      if (GetSymbol == T_SCONST) {
        dbg->file = StrDup(ComSymbol);
      } else {
        ParError("invalid logging file name");
      }
      break;
    case T_ENCODING:
      if (GetSymbol == T_SCONST) {
#if 1
        dbg->coding = StrDup(ComSymbol);
#else
        if ((stricmp(ComSymbol, "utf8") == 0) ||
            (stricmp(ComSymbol, "utf-8") == 0)) {
          dbg->coding = NULL;
        } else {
          dbg->coding = StrDup(ComSymbol);
        }
#endif
      } else {
        ParError("invalid logging file name");
      }
      break;
    case T_REDIRECT:
      if (GetSymbol == T_SCONST) {
        dbg->redirectName = StrDup(ComSymbol);
      } else {
        ParError("invalid redirect group");
      }
      break;
    case T_PRIORITY:
      if (GetSymbol == T_ICONST) {
        dbg->priority = atoi(ComSymbol);
      } else {
        ParError("priority invalid");
      }
      break;
    case T_PORT:
      if (GetSymbol == T_SCONST) {
        dbg->server->port = ParPort(ComSymbol, NULL);
      } else {
        ParError("invalid port");
      }
      break;
    case T_NAME:
      if (GetSymbol == T_SCONST) {
        dbg->server->dbname = StrDup(ComSymbol);
      } else {
        ParError("invalid DB name");
      }
      break;
    case T_USER:
      if (GetSymbol == T_SCONST) {
        dbg->server->user = StrDup(ComSymbol);
      } else {
        ParError("invalid DB user");
      }
      break;
    case T_PASS:
      if (GetSymbol == T_SCONST) {
        dbg->server->pass = StrDup(ComSymbol);
      } else {
        ParError("invalid DB password");
      }
      break;
    case T_SSLMODE:
      if (GetSymbol == T_SCONST) {
        dbg->server->sslmode = StrDup(ComSymbol);
      } else {
        ParError("invalid DB sslmode");
      }
      break;
    case T_SSLCERT:
      if (GetSymbol == T_SCONST) {
        dbg->server->sslcert = StrDup(ComSymbol);
      } else {
        ParError("invalid DB sslcert");
      }
      break;
    case T_SSLKEY:
      if (GetSymbol == T_SCONST) {
        dbg->server->sslkey = StrDup(ComSymbol);
      } else {
        ParError("invalid DB sslkey");
      }
      break;
    case T_SSLROOTCERT:
      if (GetSymbol == T_SCONST) {
        dbg->server->sslrootcert = StrDup(ComSymbol);
      } else {
        ParError("invalid DB sslrootcert");
      }
      break;
    case T_SSLCRL:
      if (GetSymbol == T_SCONST) {
        dbg->server->sslcrl = StrDup(ComSymbol);
      } else {
        ParError("invalid DB sslcrl");
      }
      break;
    case T_LOGDBNAME:
      if (GetSymbol == T_SCONST) {
        dbg->redirectName = StrDup(ComSymbol);
        dbg->redirectorMode = REDIRECTOR_MODE_LOG;
      } else {
        ParError("invalid logdbname");
      }
      break;
    case T_LOGTABLENAME:
      if (GetSymbol == T_SCONST) {
        dbg->logTableName = StrDup(ComSymbol);
      } else {
        ParError("invalid logtable_name");
      }
      break;
    case T_LOGPORT:
      if (GetSymbol == T_SCONST) {
        DestroyPort(dbg->redirectPort);
        dbg->redirectPort = ParPort(ComSymbol, PORT_LOG);
      } else {
        ParError("invalid port");
      }
      break;
    case T_AUDITLOG:
      if (GetSymbol == T_ICONST) {
        dbg->auditlog = atoi(ComSymbol);
      } else {
        ParError("auditlog invalid");
      }
      break;
    case T_SUMCHECK:
      if (GetSymbol == T_ICONST) {
        dbg->sumcheck = atoi(ComSymbol);
      } else {
        ParError("sumcheck invalid");
      }
      break;
    default:
      ParErrorPrintf("other syntax error in db_group [%s]\n", ComSymbol);
      break;
    }
    if (GetSymbol != ';') {
      ParError("; not found in db_group");
    }
    ERROR_BREAK;
  }
  RegistDBG(dbg);
}

static void _AssignDBG(CURFILE *in, char *name, DBG_Struct *dbg) {
  DBG_Struct *red;

  if (dbg->redirectName != NULL) {
    red = g_hash_table_lookup(ThisEnv->DBG_Table, dbg->redirectName);
    if (red == NULL) {
      ParError("redirect DB group not found");
    }
    dbg->redirect = red;
  }
}

static int DBGcomp(DBG_Struct **x, DBG_Struct **y) {
  return ((*x)->priority - (*y)->priority);
}

static void AssignDBG(CURFILE *in) {
  int i;
  DBG_Struct *dbg;

  qsort(ThisEnv->DBG, ThisEnv->cDBG, sizeof(DBG_Struct *),
        (int (*)(const void *, const void *))DBGcomp);
  for (i = 0; i < ThisEnv->cDBG; i++) {
    dbg = ThisEnv->DBG[i];
    dbgprintf("%d DB group name = [%s]\n", dbg->priority, dbg->name);
    dbgprintf("  redirectorMode => %d", dbg->redirectorMode);
    _AssignDBG(in, dbg->name, dbg);
  }
}

static RecordStruct *BuildAuditLog(void) {
  RecordStruct *rec;
  char *buff, *p;
  buff = (char *)xmalloc(SIZE_BUFF);
  p = buff;
  p += sprintf(p, "%s	{", AUDITLOG_TABLE);
  p += sprintf(p, "exec_date	timestamp;");
  p += sprintf(p, "username	varchar(%d);", SIZE_USER);
  p += sprintf(p, "term		varchar(%d);", SIZE_TERM);
  p += sprintf(p, "windowname varchar(%d);", SIZE_NAME);
  p += sprintf(p, "widget	varchar(%d);", SIZE_NAME);
  p += sprintf(p, "event		varchar(%d);", SIZE_NAME);
  p += sprintf(p, "comment	varchar(%d);", SIZE_COMMENT);
  ;
  p += sprintf(p, "func		varchar(%d);", SIZE_FUNC);
  p += sprintf(p, "result	integer;");
  p += sprintf(p, "ticket_id	integer;");
  p += sprintf(p, "exec_query	text;");
  sprintf(p, "};");
  rec = ParseRecordMem(buff);
  xfree(buff);
  return (rec);
}

static RecordStruct *BuildMcpArea(size_t stacksize) {
  RecordStruct *rec;
  char *buff, *p;

  buff = (char *)xmalloc(SIZE_BUFF);
  p = buff;
  p += sprintf(p, "mcparea	{");
  p += sprintf(p, "version int;");
  p += sprintf(p, "func varchar(%d);", SIZE_FUNC);
  p += sprintf(p, "rc int;");
  p += sprintf(p, "dc	{");
  p += sprintf(p, "window				varchar(%d);", SIZE_NAME);
  p += sprintf(p, "widget				varchar(%d);", SIZE_NAME);
  p += sprintf(p, "event				varchar(%d);", SIZE_NAME);
  p += sprintf(p, "module				varchar(%d);", SIZE_NAME);
  p += sprintf(p, "status				varchar(%d);", SIZE_STATUS);
  p += sprintf(p, "dbstatus			varchar(%d);", SIZE_STATUS);
  p += sprintf(p, "puttype			varchar(%d);", SIZE_PUTTYPE);
  p += sprintf(p, "tenant				varchar(%d);", SIZE_TERM);
  p += sprintf(p, "user				varchar(%d);", SIZE_USER);
  p += sprintf(p, "term				varchar(%d);", SIZE_TERM);
  p += sprintf(p, "host				varchar(%d);", SIZE_HOST);
  p += sprintf(p, "tempdir			varchar(%d);", SIZE_PATH);
  p += sprintf(p, "middleware_name	varchar(%d);", SIZE_NAME);
  p += sprintf(p, "middleware_version	varchar(%d);", SIZE_NAME);
  p += sprintf(p, "};");
  p += sprintf(p, "db	{");
  p += sprintf(p, "path	{");
  p += sprintf(p, "blocks	int;");
  p += sprintf(p, "rname	int;");
  p += sprintf(p, "pname	int;");
  p += sprintf(p, "};");
  p += sprintf(p, "table		varchar(%d);", SIZE_NAME);
  p += sprintf(p, "pathname	varchar(%d);", SIZE_NAME);
  p += sprintf(p, "limit		int;");
  p += sprintf(p, "rcount		int;");
  p += sprintf(p, "redirect	int;");
  p += sprintf(p, "logflag	int;");
  p += sprintf(p, "logcomment	varchar(%d);", SIZE_COMMENT);
  p += sprintf(p, "};");
  sprintf(p, "};");
  rec = ParseRecordMem(buff);
  xfree(buff);
  return (rec);
}

static DI_Struct *NewEnv(char *name) {
  char buff[SIZE_LONGNAME + 1];
  DI_Struct *env;

  env = New(DI_Struct);
  env->name = StrDup(name);
  env->BaseDir = BaseDir;
  env->D_Dir = D_Dir;
  env->RecordDir = RecordDir;
  sprintf(buff, "/tmp/wfc.%s", name);
  env->WfcApsPort = ParPort(buff, NULL);
  sprintf(buff, "/tmp/wfc.term");
  env->TermPort = ParPort(buff, NULL);
  sprintf(buff, "/tmp/wfcc.%s", name);
  env->ControlPort = ParPort(buff, NULL);
  env->cLD = 0;
  env->cBD = 0;
  env->cDBD = 0;
  env->stacksize = SIZE_STACK;
  env->LD_Table = NewNameHash();
  env->BD_Table = NewNameHash();
  env->DBD_Table = NewNameHash();
  env->mlevel = MULTI_NO;
  env->cDBG = 0;
  env->DBG = NULL;
  env->DBG_Table = NewNameHash();
  env->sysdata = NULL;
  env->ApsPath = NULL;
  env->WfcPath = NULL;
  env->RedPath = NULL;
  env->DbPath = NULL;
  env->linkrec = NULL;
  env->DBLoggerPath = NULL;
  env->DBMasterPath = NULL;
  env->DBSlavePath = NULL;
  env->DBMasterPort = NULL;
  env->DBMasterAuth = NULL;
  env->DBMasterLogDBName = NULL;
  env->InitialLD = NULL;
  env->CryptoPass = NULL;

  return (env);
}

static DI_Struct *ParDI(CURFILE *in, char *ld, char *bd, char *db,
                        int parse_type) {
  char gname[SIZE_LONGNAME + 1], buff[SIZE_LONGNAME + 1];

  ThisEnv = NULL;
  while (GetSymbol != T_EOF) {
    switch (ComToken) {
    case T_NAME:
      if ((GetName != T_SYMBOL) && (ComToken != T_SCONST)) {
        ParError("no name");
      } else {
        ThisEnv = NewEnv(ComSymbol);
      }
      break;
    case T_STACKSIZE:
      if (GetSymbol == T_ICONST) {
        if (ComInt > SIZE_STACK) {
          ThisEnv->stacksize = ComInt;
        }
      } else {
        ParError("stacksize must be integer");
      }
      break;
    case T_LINKSIZE:
      ParError("this feature is obsolete. use linkage directive");
      break;
    case T_LINKAGE:
      if (GetSymbol == T_SYMBOL) {
        if (parse_type >= P_LD) {
          sprintf(buff, "%s.rec", ComSymbol);
          ThisEnv->linkrec = ReadRecordDefine(buff,TRUE);
        } else {
          break;
        }
        if (ThisEnv->linkrec == NULL) {
          ParError("linkage record not found");
        }
      } else {
        ParError("linkage invalid");
      }
      break;
    case T_BASEDIR:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->BaseDir == NULL) {
          if (!strcmp(ComSymbol, ".")) {
            ThisEnv->BaseDir = ".";
          } else {
            ThisEnv->BaseDir = StrDup(ExpandPath(ComSymbol, NULL));
          }
        }
      } else {
        ParError("base directory invalid");
      }
      break;
    case T_RECDIR:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->RecordDir == NULL) {
          if (!strcmp(ComSymbol, ".")) {
            ThisEnv->RecordDir = StrDup(ThisEnv->BaseDir);
          } else {
            ThisEnv->RecordDir =
                StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
          }
          RecordDir = ThisEnv->RecordDir;
        }
      } else {
        ParError("recored directory invalid");
      }
      break;
    case T_DDIR:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->D_Dir == NULL) {
          if (!strcmp(ComSymbol, ".")) {
            ThisEnv->D_Dir = StrDup(ThisEnv->BaseDir);
          } else {
            ThisEnv->D_Dir = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
          }
        }
      } else {
        ParError("DDIR directory invalid");
      }
      break;
    case T_APSPATH:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->ApsPath == NULL) {
          ThisEnv->ApsPath = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ParError("APS path definision duplicate");
        }
      } else {
        ParError("APS path invalid");
      }
      break;
    case T_WFCPATH:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->WfcPath == NULL) {
          ThisEnv->WfcPath = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ParError("WFC path definision duplicate");
        }
      } else {
        ParError("WFC path invalid");
      }
      break;
    case T_REDPATH:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->RedPath == NULL) {
          ThisEnv->RedPath = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ParError("DBREDIRECTOR path definision duplicate");
        }
      } else {
        ParError("DBREDIRECTOR path invalid");
      }
      break;
    case T_DBPATH:
      if (GetSymbol == T_SCONST) {
        ThisEnv->DbPath = StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        SetDBGPath(ThisEnv->DbPath);
      } else {
        ParError("db handler load path invalid");
      }
      break;
    case T_MULTI:
      GetName;
      if (stricmp(ComSymbol, "no") == 0) {
        ThisEnv->mlevel = MULTI_NO;
      } else if (stricmp(ComSymbol, "db") == 0) {
        ThisEnv->mlevel = MULTI_DB;
      } else if (stricmp(ComSymbol, "ld") == 0) {
        ThisEnv->mlevel = MULTI_LD;
      } else if (stricmp(ComSymbol, "id") == 0) {
        ThisEnv->mlevel = MULTI_ID;
      } else if (stricmp(ComSymbol, "aps") == 0) {
        ThisEnv->mlevel = MULTI_APS;
      } else {
        ParError("invalid multiplex level");
      }
      break;
    case T_BLOB:
    case T_SYSDATA:
      if (GetSymbol == '{') {
        ThisEnv->sysdata = ParSysData(in);
      } else {
        ParError("syntax error in blob directive");
      }
      break;
    case T_CONTROL:
      if (GetSymbol == '{') {
        ParCONTROL(in);
      } else {
        ParError("syntax error in control directive");
      }
      break;
    case T_WFC:
      if (GetSymbol == '{') {
        ParWFC(in);
      } else {
        ParError("syntax error in wfc directive");
      }
      break;
    case T_LD:
      if (GetSymbol == '{') {
        ParLD(in, ld, parse_type);
      } else {
        ParError("syntax error in ld directive");
      }
      break;
    case T_INITIAL_LD:
      GetName;
      ThisEnv->InitialLD = StrDup(ComSymbol);
      break;
    case T_BD:
      if (GetSymbol == '{') {
        ParBD(in, bd, parse_type);
      } else {
        ParError("syntax error in bd directive");
      }
      break;
    case T_DB:
      if (GetSymbol == '{') {
        ParDBD(in, db);
      } else {
        ParError("syntax error in db directive");
      }
      break;
    case T_DBGROUP:
      if (GetSymbol == T_SCONST) {
        strcpy(gname, ComSymbol);
        if (GetSymbol != '{') {
          ParError("syntax error in db names");
        }
      } else if (ComToken == '{') {
        strcpy(gname, "");
      } else {
        ParError("syntax error dbgroup directive");
      }
      ParDBGROUP(in, gname);
      break;
    case T_LOGPATH:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->DBLoggerPath == NULL) {
          ThisEnv->DBLoggerPath =
              StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ParError("DBLOG path definision duplicate");
        }
      } else {
        ParError("DBLOG path invalid");
      }
      break;
    case T_MSTPATH:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->DBMasterPath == NULL) {
          ThisEnv->DBMasterPath =
              StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ParError("DBMASTER path definision duplicate");
        }
      } else {
        ParError("DBMASTER path invalid");
      }
      break;
    case T_SLVPATH:
      if (GetSymbol == T_SCONST) {
        if (ThisEnv->DBSlavePath == NULL) {
          ThisEnv->DBSlavePath =
              StrDup(ExpandPath(ComSymbol, ThisEnv->BaseDir));
        } else {
          ParError("DBSLAVE path definision duplicate");
        }
      } else {
        ParError("DBSLAVE path invalid");
      }
      break;
    case T_DBMASTER:
      if (GetSymbol == '{') {
        ParDBMaster(in);
      } else {
        ParError("syntax error in dbmaster directive");
      }
      break;
    case T_CRYPTOPASS:
      GetName;
      ThisEnv->CryptoPass = StrDup(ComSymbol);
      break;
    default:
      ParErrorPrintf("misc syntax error [%X][%s]\n", ComToken, ComSymbol);
      break;
    }
    if (GetSymbol != ';') {
      ParError("; missing ParDI");
    }
    ERROR_BREAK;
  }
  if (ThisEnv != NULL) {
    ThisEnv->mcprec = BuildMcpArea(ThisEnv->stacksize);
    ThisEnv->auditrec = BuildAuditLog();
    AssignDBG(in);
  }
  return (ThisEnv);
}

extern DI_Struct *DI_Parser(char *name, char *ld, char *bd, char *db,
                            Bool parse_ld) {
  struct stat stbuf;
  DI_Struct *ret;
  CURFILE *in, root;

  ret = NULL;
  root.next = NULL;
  DirectoryDir = dirname(StrDup(name));
  if (stat(name, &stbuf) == 0) {
    if ((in = PushLexInfo(&root, name, DirectoryDir, Reserved)) != NULL) {
      ret = ParDI(in, ld, bd, db, parse_ld);
      DropLexInfo(&in);
    }
  } else {
    Error("DI file not found %s\n", name);
  }
  return (ret);
}

extern void DI_ParserInit(void) {
  LexInit();
  Reserved = MakeReservedTable(tokentable);
}
