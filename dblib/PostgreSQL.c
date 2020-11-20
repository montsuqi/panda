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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_POSTGRES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <numeric.h>

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

#include "const.h"
#include "enum.h"
#include "SQLparser.h"
#include "libmondai.h"
#include "dbgroup.h"
#include "directory.h"
#include "message.h"
#include "debug.h"
#include "PostgreSQLlib.h"

static int level;
static char *rname[SIZE_RNAME];
static int alevel;
static int Dim[SIZE_RNAME];
/*マルチフェッチ結果*/
static PGresult *fetch_result = NULL;
static int fetch_index = 0;

static void _PQclear(PGresult *res);

static void EscapeString(DBG_Struct *dbg, LargeByteString *lbs, char *str) {
  int error;
  size_t len;
  size_t size;

  if (str == NULL)
    return;
  len = strlen(str);
  if (len == 0)
    return;

  size = LBS_Size(lbs);
  LBS_ReserveSize(lbs, size + (len * 2) + 1, TRUE);
  size += PQescapeStringConn(PGCONN(dbg), LBS_Body(lbs) + size, str,
                             len, &error);
  if (error == 0) {
    LBS_ReserveSize(lbs, size, TRUE);
    LBS_SetPos(lbs, size);
  } else {
    Warning("PostgreSQL: %s", PQerrorMessage(PGCONN(dbg)));
  }
}

static void EscapeBytea(DBG_Struct *dbg, LargeByteString *lbs,
                        unsigned char *bintext, size_t binlen) {
  size_t old_size;
  size_t to_length;
  unsigned char *dp;
  unsigned char *to_char;

  old_size = LBS_Size(lbs);
  to_char =
      PQescapeByteaConn(PGCONN(dbg), bintext, binlen, &to_length);
  if (to_char != NULL) {
    LBS_ReserveSize(lbs, old_size + to_length, TRUE);
    dp = LBS_Body(lbs) + old_size;
    memcpy(dp, to_char, to_length);
    PQfreemem(to_char);
    LBS_SetPos(lbs, old_size + to_length);
  } else {
    Warning("PostgreSQL: %s", PQerrorMessage(PGCONN(dbg)));
  }
}

static ValueStruct *UnEscapeBytea(unsigned char *from) {
  ValueStruct *bin;
  unsigned char *bintext;
  size_t to_length;

  bin = NewValue(GL_TYPE_BINARY);
  bintext = PQunescapeBytea(from, &to_length);
  SetValueBinary(bin, bintext, to_length);
  PQfreemem(bintext);
  return bin;
}

static void NoticeMessage(void *arg, const char *message) {
  Warning("%s", message);
}

static int cmdTuples(PGresult *res) {
  char *cmdtuples;
  int ret = 0;

  cmdtuples = PQcmdTuples(res);
  if (*cmdtuples != '\0') {
    ret = atoi(cmdtuples);
  }
  return ret;
}

static Bool ExistFunc(PGconn *conn, char *func) {
  char sql[SIZE_SQL + 1];
  PGresult *res;
  Bool ret = FALSE;

  sprintf(sql, "SELECT proname FROM pg_proc WHERE proname = '%s';", func);
  res = PQexec(conn, sql);
  if ((res != NULL) || (PQresultStatus(res) == PGRES_TUPLES_OK)) {
    if (PQntuples(res) > 0) {
      ret = TRUE;
    }
    PQclear(res);
  }
  return ret;
}

static void PgInitConnect(PGconn *conn) {
  PGresult *res;

  res = NULL;
  PQsetNoticeProcessor(conn, NoticeMessage, NULL);
  if (PQserverVersion(conn) >= 80200) {
    res = PQexec(conn, "SET STANDARD_CONFORMING_STRINGS = ON;");
    if ((res == NULL) || (PQresultStatus(res) != PGRES_COMMAND_OK)) {
      Warning("PostgreSQL: %s", PQerrorMessage(conn));
    }
    PQclear(res);
  }
  res = PQexec(conn, "SET SESSION CHARACTERISTICS AS TRANSACTION ISOLATION "
                     "LEVEL REPEATABLE READ;");
  if ((res == NULL) || (PQresultStatus(res) != PGRES_COMMAND_OK)) {
    Warning("PostgreSQL: %s", PQerrorMessage(conn));
  }
  PQclear(res);
}

static void CryptoMode(DBG_Struct *dbg) {
  char *crypto;
  char sql[SIZE_SQL + 1];
  PGresult *res;

  if ((crypto = GetDB_Crypt(dbg)) != NULL) {
    if (ExistFunc(PGCONN(dbg), "crypto_mode")) {
      sprintf(sql, "SELECT crypto_mode('%s');", crypto);
      res = PQexec(PGCONN(dbg), sql);
      PQclear(res);
    }
  }
}

static void SetDBGcoding(DBG_Struct *dbg, char *encoding) {
  if (encoding) {
    if (dbg->coding != NULL) {
      xfree(dbg->coding);
    }
    if ((stricmp(encoding, "utf8") == 0)) {
      dbg->coding = NULL;
    } else if ((stricmp(encoding, "euc_jp") == 0)) {
      dbg->coding = StrDup("euc-jisx0213");
    } else {
      dbg->coding = StrDup(encoding);
    }
  }
}

static Bool InTrans(PGconn *conn) {
  Bool rc = FALSE;
  if ((PQtransactionStatus(conn) == PQTRANS_INTRANS) ||
      (PQtransactionStatus(conn) == PQTRANS_ACTIVE)) {
    rc = TRUE;
  }
  return rc;
}

static void ValueToSQL(DBG_Struct *dbg, LargeByteString *lbs,
                       ValueStruct *val) {
  char buff[SIZE_OTHER];
  Numeric nv;
  char *str;

  if (IS_VALUE_NIL(val)) {
    LBS_EmitString(lbs, "null");
  } else
    switch (ValueType(val)) {
    case GL_TYPE_BYTE:
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_TEXT:
    case GL_TYPE_OBJECT:
      LBS_EmitChar(lbs, '\'');
      EscapeString(dbg, lbs, ValueToString(val, dbg->coding));
      LBS_EmitChar(lbs, '\'');
      break;
    case GL_TYPE_BINARY:
      LBS_EmitChar(lbs, '\'');
      EscapeBytea(dbg, lbs, ValueByte(val), ValueByteLength(val));
      LBS_EmitChar(lbs, '\'');
      break;
    case GL_TYPE_DBCODE:
      LBS_EmitString(lbs, ValueToString(val, dbg->coding));
      break;
    case GL_TYPE_NUMBER:
      nv = FixedToNumeric(ValueFixed(val));
      str = NumericOutput(nv);
      LBS_EmitString(lbs, str);
      xfree(str);
      NumericFree(nv);
      break;
    case GL_TYPE_INT:
      snprintf(buff, sizeof(buff), "%d", ValueToInteger(val));
      LBS_EmitString(lbs, buff);
      break;
    case GL_TYPE_FLOAT:
      snprintf(buff, sizeof(buff), "%g", ValueToFloat(val));
      LBS_EmitString(lbs, buff);
      break;
    case GL_TYPE_BOOL:
      snprintf(buff, sizeof(buff), "'%s'", ValueToBool(val) ? "t" : "f");
      LBS_EmitString(lbs, buff);
      break;
    case GL_TYPE_TIMESTAMP:
      snprintf(buff, sizeof(buff), "timestamp '%d-%d-%d %d:%d:%d'",
               ValueDateTimeYear(val) + 1900, ValueDateTimeMon(val) + 1,
               ValueDateTimeMDay(val), ValueDateTimeHour(val),
               ValueDateTimeMin(val), ValueDateTimeSec(val));
      LBS_EmitString(lbs, buff);
      break;
    case GL_TYPE_DATE:
      snprintf(buff, sizeof(buff), "date '%d-%d-%d'",
               ValueDateTimeYear(val) + 1900, ValueDateTimeMon(val) + 1,
               ValueDateTimeMDay(val));
      LBS_EmitString(lbs, buff);
      break;
    case GL_TYPE_TIME:
      snprintf(buff, sizeof(buff), "time '%d:%d:%d'", ValueDateTimeHour(val),
               ValueDateTimeMin(val), ValueDateTimeSec(val));
      LBS_EmitString(lbs, buff);
      break;
    default:
      break;
    }
}

static void KeyValue(DBG_Struct *dbg, LargeByteString *lbs, ValueStruct *args,
                     char **pk) {
  ValueStruct *val;

  val = args;
  while (*pk != NULL) {
    val = GetRecordItem(args, *pk);
    pk++;
  }
  ValueToSQL(dbg, lbs, val);
}

static char *ItemName(void) {
  static char buff[SIZE_LONGNAME];
  char *p;
  int i;

  p = buff;
  if (level > 1) {
    for (i = 0; i < level - 1; i++) {
      p += sprintf(p, "%s_", rname[i]);
    }
  }
  sprintf(p, "%s", rname[level - 1]);
  dbgprintf("item = [%s]", buff);
  return (buff);
}

static char *ParArray(DBG_Struct *dbg, char *p, ValueStruct *val) {
  ValueStruct *item;
  Bool fMinus;
  size_t len;
  char *pp, *q, *qq;
  int i, j;
  int ival;

  if (*p == '{') {
    p = ParArray(dbg, p + 1, val);
    if (*p == '}') {
      p++;
    }
  } else {
    for (i = 0; i < ValueArraySize(val); i++) {
      item = ValueArrayItem(val, i);
      switch (ValueType(item)) {
      case GL_TYPE_INT:
        if (*p == '-') {
          fMinus = TRUE;
          p++;
        } else {
          fMinus = FALSE;
        }
        ival = 0;
        while (isdigit(*p)) {
          ival *= 10;
          ival += (*p - '0');
          p++;
        }
        ival = (fMinus) ? -ival : ival;
        SetValueInteger(item, ival);
        break;
      case GL_TYPE_BOOL:
        SetValueBool(item, *p == 't');
        p++;
        break;
      case GL_TYPE_OBJECT:
      case GL_TYPE_BYTE:
      case GL_TYPE_CHAR:
      case GL_TYPE_VARCHAR:
        if (*p == '"') {
          p++;
          q = ValueToString(item, dbg->coding);
          len = 0;
          while (*p != '"') {
            if (len < ValueStringLength(item)) {
              len++;
              *q++ = *p;
            }
            p++;
          }
          *q = 0;
          p++;
        }
        break;
      case GL_TYPE_TEXT:
        pp = p;
        if (*p == '"') {
          p++;
          len = 0;
          while (*p != '"') {
            p++;
            len++;
          }
        } else
          break;
        qq = (char *)xmalloc(len + 1);
        p = pp + 1;
        q = qq;
        while (*p != '"') {
          *q++ = *p++;
        }
        *q = 0;
        p++;
        SetValueString(item, qq, dbg->coding);
        xfree(qq);
        break;
      case GL_TYPE_BINARY:
        pp = p;
        if (*p == '"') {
          p++;
          len = 0;
          while (*p != '"') {
            p++;
            len++;
          }
        } else
          break;
        qq = (char *)xmalloc(len + 1);
        p = pp + 1;
        q = qq;
        while (*p != '"') {
          *q++ = *p++;
        }
        *q = 0;
        p++;
        SetValueBinary(item, qq, q - qq);
        xfree(qq);
        break;
      case GL_TYPE_ARRAY:
        p = ParArray(dbg, p, item);
        break;
      case GL_TYPE_ROOT_RECORD:
      case GL_TYPE_RECORD:
        for (j = 0; j < ValueRecordSize(item); j++) {
          p = ParArray(dbg, p, ValueRecordItem(item, i));
        }
        break;
      case GL_TYPE_DBCODE:
      default:
        break;
      }
      while (isspace(*p))
        p++;
      if (*p == ',')
        p++;
      while (isspace(*p))
        p++;
    }
  }
  return (p);
}

#define STATE_DATE_NULL 0
#define STATE_DATE_YEAR 1
#define STATE_DATE_MON 2
#define STATE_DATE_MDAY 3
#define STATE_DATE_HOUR 4
#define STATE_DATE_MIN 5
#define STATE_DATE_SEC 6
static void ParseDate(ValueStruct *val, char *str, int state) {
  char *p;

  InitializeValue(val);
  while (*str != 0) {
    ValueIsNonNil(val);
    switch (state) {
    case STATE_DATE_YEAR:
      if ((p = strchr(str, '-')) != NULL) {
        *p = 0;
        state = STATE_DATE_MON;
      } else {
        p = str;
        state = STATE_DATE_NULL;
      }
      ValueDateTimeYear(val) = atoi(str);
      str = p + 1;
      break;
    case STATE_DATE_MON:
      if ((p = strchr(str, '-')) != NULL) {
        *p = 0;
        state = STATE_DATE_MDAY;
      } else {
        p = str;
        state = STATE_DATE_NULL;
      }
      ValueDateTimeMon(val) = atoi(str) - 1;
      str = p + 1;
      break;
    case STATE_DATE_MDAY:
      if ((p = strchr(str, ' ')) != NULL) {
        *p = 0;
        state = STATE_DATE_HOUR;
      } else {
        p = str;
        state = STATE_DATE_NULL;
      }
      ValueDateTimeMDay(val) = atoi(str);
      str = p + 1;
      break;
    case STATE_DATE_HOUR:
      if ((p = strchr(str, ':')) != NULL) {
        *p = 0;
        state = STATE_DATE_MIN;
      } else {
        p = str;
        state = STATE_DATE_NULL;
      }
      ValueDateTimeHour(val) = atoi(str);
      str = p + 1;
      break;
    case STATE_DATE_MIN:
      if ((p = strchr(str, ':')) != NULL) {
        *p = 0;
        state = STATE_DATE_SEC;
      } else {
        p = str;
        state = STATE_DATE_NULL;
      }
      ValueDateTimeMin(val) = atoi(str);
      str = p + 1;
      break;
    case STATE_DATE_SEC:
      ValueDateTimeSec(val) = atoi(str);
      state = STATE_DATE_NULL;
      break;
    default:
      break;
    }
    if (state == STATE_DATE_NULL)
      break;
  }
}

static void GetTable(DBG_Struct *dbg, PGresult *res, int ix, ValueStruct *val) {
  int i;
  ValueStruct *tmp;
  int fnum;
  Numeric nv;
  char buff[SIZE_OTHER];
  char *str;

  if (val == NULL)
    return;

  switch (ValueType(val)) {
  case GL_TYPE_INT:
    dbgmsg("int");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      SetValueInteger(val, atoi(PQgetvalue(res, ix, fnum)));
    }
    break;
  case GL_TYPE_BOOL:
    dbgmsg("bool");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      SetValueBool(val, *PQgetvalue(res, ix, fnum) == 't');
    }
    break;
  case GL_TYPE_TIMESTAMP:
    dbgmsg("timestamp");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      strcpy(buff, PQgetvalue(res, ix, fnum));
      ParseDate(val, buff, STATE_DATE_YEAR);
    }
    break;
  case GL_TYPE_DATE:
    dbgmsg("date");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      strcpy(buff, PQgetvalue(res, ix, fnum));
      ParseDate(val, buff, STATE_DATE_YEAR);
    }
    break;
  case GL_TYPE_TIME:
    dbgmsg("time");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      strcpy(buff, PQgetvalue(res, ix, fnum));
      ParseDate(val, buff, STATE_DATE_HOUR);
    }
    break;
  case GL_TYPE_OBJECT:
  case GL_TYPE_BYTE:
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_DBCODE:
  case GL_TYPE_TEXT:
    dbgmsg("char");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      SetValueString(val, PQgetvalue(res, ix, fnum), dbg->coding);
    }
    break;
  case GL_TYPE_BINARY:
    dbgmsg("byte");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      SetValueBinary(val, PQgetvalue(res, ix, fnum),
                     PQgetlength(res, ix, fnum));
    }
    break;
  case GL_TYPE_NUMBER:
    dbgmsg("number");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      nv = NumericInput(PQgetvalue(res, ix, fnum), ValueFixedLength(val),
                        ValueFixedSlen(val));
      str = NumericOutput(nv);
      SetValueString(val, str, NULL);
      xfree(str);
      NumericFree(nv);
    }
    break;
  case GL_TYPE_ARRAY:
    dbgmsg("array");
    fnum = PQfnumber(res, ItemName());
    if (fnum < 0) {
      if (!IS_VALUE_VIRTUAL(val)) {
        ValueIsNil(val);
      }
    } else {
      ParArray(dbg, PQgetvalue(res, ix, fnum), val);
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    level++;
    dbgmsg(">record");
    for (i = 0; i < ValueRecordSize(val); i++) {
      tmp = ValueRecordItem(val, i);
      rname[level - 1] = ValueRecordName(val, i);
      GetTable(dbg, res, ix, tmp);
    }
    dbgmsg("<record");
    level--;
    break;
  case GL_TYPE_ALIAS:
    Warning("invalid data type");
    break;
  default:
    break;
  }
}

static void GetValue(DBG_Struct *dbg, PGresult *res, int tnum, int fnum,
                     ValueStruct *val) {
  Numeric nv;
  char *str;
  char buff[SIZE_OTHER];

  if (val == NULL)
    return;

  if (PQgetisnull(res, tnum, fnum) == 1) { /*	null	*/
    ValueIsNil(val);
  } else {
    ValueIsNonNil(val);
    switch (ValueType(val)) {
    case GL_TYPE_INT:
      SetValueInteger(val, atoi(PQgetvalue(res, tnum, fnum)));
      break;
    case GL_TYPE_BOOL:
      SetValueBool(val, *PQgetvalue(res, tnum, fnum) == 't');
      break;
    case GL_TYPE_TIMESTAMP:
      strcpy(buff, PQgetvalue(res, tnum, fnum));
      ParseDate(val, buff, STATE_DATE_YEAR);
      break;
    case GL_TYPE_DATE:
      strcpy(buff, PQgetvalue(res, tnum, fnum));
      ParseDate(val, buff, STATE_DATE_YEAR);
      break;
    case GL_TYPE_TIME:
      strcpy(buff, PQgetvalue(res, tnum, fnum));
      ParseDate(val, buff, STATE_DATE_HOUR);
      break;
    case GL_TYPE_OBJECT:
    case GL_TYPE_BYTE:
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_DBCODE:
    case GL_TYPE_TEXT:
      SetValueString(val, PQgetvalue(res, tnum, fnum), dbg->coding);
      break;
    case GL_TYPE_BINARY:
      SetValueBinary(val, PQgetvalue(res, tnum, fnum),
                     PQgetlength(res, tnum, fnum));
      break;
    case GL_TYPE_NUMBER:
      nv = NumericInput(PQgetvalue(res, tnum, fnum), ValueFixedLength(val),
                        ValueFixedSlen(val));
      str = NumericToFixed(nv, ValueFixedLength(val), ValueFixedSlen(val));
      strcpy(ValueFixedBody(val), str);
      xfree(str);
      NumericFree(nv);
      break;
    case GL_TYPE_ARRAY:
      ParArray(dbg, PQgetvalue(res, tnum, fnum), val);
      break;
    case GL_TYPE_ROOT_RECORD:
    case GL_TYPE_RECORD:
      break;
    default:
      break;
    }
  }
}

static char *PutDim(void) {
  static char buff[SIZE_NAME + 1];
  int i;
  char *p;

  p = buff;
  *p = 0;
  for (i = 0; i < alevel; i++) {
    p += sprintf(p, "[%d]", Dim[i] + 1);
  }
  return (buff);
}

static void ClearFetchResult() {
  if (fetch_result != NULL) {
    _PQclear(fetch_result);
  }
  fetch_result = NULL;
  fetch_index = 0;
}

static ValueStruct *_PGresToValue(DBG_Struct *dbg, PGresult *res, int count,
                                  ValueStruct *val) {
  ValueStruct *ret;

  ret = DuplicateValue(val, FALSE);
  level = 0;
  alevel = 0;
  GetTable(dbg, res, count, ret);
  return ret;
}

static ValueStruct *PGresToValueFetch(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                 ValueStruct *val) {
  int count;
  ValueStruct *ret;

  ret = NULL;
  if (ctrl->rc != MCP_OK) {
    return ret;
  }
  if (ctrl->limit < 1) {
    Warning("Invalid ctrl->limit:%d",ctrl->limit);
    ctrl->rc = MCP_BAD_OTHER;
    return ret;
  }
  if (fetch_result == NULL) {
    Warning("fetch_result is null");
    ctrl->rc = MCP_BAD_OTHER;
    return ret;
  }
  count = PQntuples(fetch_result);
  if (count <= 0) {
    ClearFetchResult();
    ctrl->rc = MCP_EOF;
    return ret;
  }
  ret = _PGresToValue(dbg, fetch_result, fetch_index, val);
  fetch_index += 1;
  if (fetch_index >= count) {
    /* refetch */
    ClearFetchResult();
  }
  return ret;
}

static ValueStruct *PGresToValue(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                 PGresult *res, ValueStruct *val) {
  int i;
  ValueStruct *value, *ret;

  ret = NULL;
  if (ctrl->rc != MCP_OK) {
    return ret;
  }
  ctrl->rcount = PQntuples(res);
  if (ctrl->rcount > 0) {
    if (ctrl->limit > 1) {
      ret = NewValue(GL_TYPE_ARRAY);
      for (i = 0; i < ctrl->rcount; i++) {
        value = _PGresToValue(dbg, res, i, val);
        ValueAddArrayItem(ret, i, value);
      }
    } else {
      ret = _PGresToValue(dbg, res, 0, val);
    }
  } else {
    ctrl->rc = MCP_EOF;
  }
  return ret;
}

static ValueStruct *_PGresToValue2(DBG_Struct *dbg, int tnum, PGresult *res) {
  ValueStruct *value, *tuple;
  int i, fnum;
  char *str;

  fnum = PQnfields(res);
  tuple = NewValue(GL_TYPE_RECORD);
  for (i = 0; i < fnum; i++) {
    value = NewValue(GL_TYPE_VARCHAR);
    str = PQgetvalue(res, tnum, i);
    SetValueString(value, str, dbg->coding);
    ValueAddRecordItem(tuple, PQfname(res, i), value);
  }
  return tuple;
}

static ValueStruct *PGresToValue2(DBG_Struct *dbg, PGresult *res) {
  ValueStruct *ret, *tuple;
  int i, tnum;

  tnum = PQntuples(res);
  if (tnum <= 0) {
    return NULL;
  }
  if (tnum == 1) {
    ret = _PGresToValue2(dbg, 0, res);
  } else {
    ret = NewValue(GL_TYPE_ARRAY);
    for (i = 0; i < tnum; i++) {
      tuple = _PGresToValue2(dbg, i, res);
      ValueAddArrayItem(ret, i, tuple);
    }
  }
  return ret;
}

static void UpdateValue(DBG_Struct *dbg, LargeByteString *lbs,
                        ValueStruct *val) {
  int i;
  ValueStruct *tmp;
  Bool fComm;

  if (val == NULL)
    return;
  if (IS_VALUE_NIL(val)) {
    LBS_EmitString(lbs, ItemName());
    LBS_EmitString(lbs, PutDim());
#if 1
    LBS_EmitString(lbs, " = null ");
#else
    LBS_EmitString(lbs, " is null ");
#endif
  } else
    switch (ValueType(val)) {
    case GL_TYPE_INT:
    case GL_TYPE_BOOL:
    case GL_TYPE_BYTE:
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_DBCODE:
    case GL_TYPE_NUMBER:
    case GL_TYPE_TEXT:
    case GL_TYPE_BINARY:
    case GL_TYPE_TIMESTAMP:
    case GL_TYPE_DATE:
    case GL_TYPE_TIME:
    case GL_TYPE_OBJECT:
      LBS_EmitString(lbs, ItemName());
      LBS_EmitString(lbs, PutDim());
      LBS_EmitString(lbs, " = ");
      ValueToSQL(dbg, lbs, val);
      LBS_EmitString(lbs, " ");
      break;
    case GL_TYPE_ARRAY:
      fComm = FALSE;
      for (i = 0; i < ValueArraySize(val); i++) {
        tmp = ValueArrayItem(val, i);
        if (!IS_VALUE_VIRTUAL(tmp)) {
          if (fComm) {
            LBS_EmitChar(lbs, ',');
          }
          fComm = TRUE;
          Dim[alevel] = i;
          alevel++;
          UpdateValue(dbg, lbs, tmp);
          alevel--;
        }
      }
      break;
    case GL_TYPE_ROOT_RECORD:
    case GL_TYPE_RECORD:
      level++;
      fComm = FALSE;
      for (i = 0; i < ValueRecordSize(val); i++) {
        tmp = ValueRecordItem(val, i);
        if (!IS_VALUE_VIRTUAL(tmp)) {
          if (fComm) {
            LBS_EmitChar(lbs, ',');
          }
          fComm = TRUE;
          rname[level - 1] = ValueRecordName(val, i);
          UpdateValue(dbg, lbs, tmp);
        }
      }
      level--;
      break;
    default:
      break;
    }
}

static void InsertNames(LargeByteString *lbs, ValueStruct *val) {
  int i;
  ValueStruct *tmp;
  Bool fComm;

  if (val == NULL) {
  } else
    switch (ValueType(val)) {
    case GL_TYPE_INT:
    case GL_TYPE_BOOL:
    case GL_TYPE_BYTE:
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_DBCODE:
    case GL_TYPE_NUMBER:
    case GL_TYPE_TEXT:
    case GL_TYPE_BINARY:
    case GL_TYPE_OBJECT:
    case GL_TYPE_TIMESTAMP:
    case GL_TYPE_DATE:
    case GL_TYPE_TIME:
    case GL_TYPE_ARRAY:
      LBS_EmitString(lbs, ItemName());
      LBS_EmitString(lbs, PutDim());
      break;
    case GL_TYPE_ROOT_RECORD:
    case GL_TYPE_RECORD:
      level++;
      fComm = FALSE;
      for (i = 0; i < ValueRecordSize(val); i++) {
        tmp = ValueRecordItem(val, i);
        if (!IS_VALUE_VIRTUAL(tmp)) {
          if (fComm) {
            LBS_EmitChar(lbs, ',');
          }
          fComm = TRUE;
          rname[level - 1] = ValueRecordName(val, i);
          InsertNames(lbs, tmp);
        }
      }
      level--;
      break;
    default:
      break;
    }
}

static void InsertValues(DBG_Struct *dbg, LargeByteString *lbs,
                         ValueStruct *val) {
  int i;
  ValueStruct *tmp;
  Bool fComm;

  if (val == NULL)
    return;
  if (IS_VALUE_NIL(val)) {
    LBS_EmitString(lbs, "null");
  } else
    switch (ValueType(val)) {
    case GL_TYPE_INT:
    case GL_TYPE_BOOL:
    case GL_TYPE_BYTE:
    case GL_TYPE_CHAR:
    case GL_TYPE_VARCHAR:
    case GL_TYPE_DBCODE:
    case GL_TYPE_NUMBER:
    case GL_TYPE_OBJECT:
    case GL_TYPE_TEXT:
    case GL_TYPE_BINARY:
    case GL_TYPE_TIMESTAMP:
    case GL_TYPE_DATE:
    case GL_TYPE_TIME:
      ValueToSQL(dbg, lbs, val);
      break;
    case GL_TYPE_ARRAY:
      LBS_EmitString(lbs, "ARRAY[");
      fComm = FALSE;
      for (i = 0; i < ValueArraySize(val); i++) {
        tmp = ValueArrayItem(val, i);
        if (!IS_VALUE_VIRTUAL(tmp)) {
          if (fComm) {
            LBS_EmitChar(lbs, ',');
          }
          fComm = TRUE;
          Dim[alevel] = i;
          alevel++;
          InsertValues(dbg, lbs, tmp);
          alevel--;
        }
      }
      LBS_EmitString(lbs, "] ");
      break;
    case GL_TYPE_ROOT_RECORD:
    case GL_TYPE_RECORD:
      level++;
      fComm = FALSE;
      for (i = 0; i < ValueRecordSize(val); i++) {
        tmp = ValueRecordItem(val, i);
        if (!IS_VALUE_VIRTUAL(tmp)) {
          if (fComm) {
            LBS_EmitString(lbs, ", ");
          }
          fComm = TRUE;
          rname[level - 1] = ValueRecordName(val, i);
          InsertValues(dbg, lbs, tmp);
        }
      }
      level--;
      break;
    default:
      break;
    }
}

static void _PQclear(PGresult *res) {
  if (res != NULL) {
    PQclear(res);
  }
}

static PGresult *_PQexec(DBG_Struct *dbg, char *sql) {
  PGresult *res;
  unsigned long t1,t2;

  dbgprintf("%s", sql);
  t1 = GetNowTime();
  res = PQexecParams(PGCONN(dbg), sql, 0, NULL, NULL, NULL, NULL, 0);
  t2 = GetNowTime();
  DbExecTime += (t2 - t1);
  if (sql) {
    LBS_String(dbg->last_query, sql);
  }
  return (res);
}

static int _PQsendQuery(DBG_Struct *dbg, char *sql) {
  dbgprintf("%s", sql);
  return PQsendQuery(PGCONN(dbg), sql);
}

static PGresult *_PQgetResult(DBG_Struct *dbg) {
  return PQgetResult(PGCONN(dbg));
}
static Bool CheckConnect(DBG_Struct *dbg) {
  if (PQstatus(PGCONN(dbg)) != CONNECTION_OK) {
    Warning("PostgreSQL: Unconnected.");
    dbg->dbstatus = DB_STATUS_UNCONNECT;
    return FALSE;
  }
  return TRUE;
}

static int CheckResult(DBG_Struct *dbg, PGresult *res, int status) {
  int rc;
  if (!CheckConnect(dbg)) {
    rc = MCP_BAD_CONN;
  } else if (res && (PQresultStatus(res) == status)) {
    dbgmsg("OK");
    rc = MCP_OK;
  } else {
    Warning("PostgreSQL: %s", PQerrorMessage(PGCONN(dbg)));
    rc = MCP_BAD_OTHER;
  }
  return rc;
}

static ValueStruct *ExecPGSQL(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              LargeByteString *src, ValueStruct *args) {
  LargeByteString *sql;
  int c;
  ValueStruct *val, *ret, *value;
  PGresult *res;
  ValueStruct **tuple;
  int n, i, j, items, ntuples;
  ExecStatusType status;
  Bool fIntoAster, fDot;
  char buff[SIZE_LONGNAME + 1], *p, *q;

  sql = NewLBS();
  if (src == NULL) {
    Error("function \"%s\" is not found.", ctrl->func);
  }
  RewindLBS(src);
  items = 0;
  tuple = NULL;
  fIntoAster = FALSE;
  ret = NULL;
  while ((c = LBS_FetchByte(src)) >= 0) {
    if (c != SQL_OP_ESC) {
      LBS_EmitChar(sql, c);
    } else {
      c = LBS_FetchByte(src);
      switch (c) {
      case SQL_OP_SYMBOL:
        p = buff;
        while ((c = LBS_FetchByte(src)) != ' ') {
          *p++ = c;
        }
        *p = 0;
        fDot = FALSE;
        p = buff;
        while ((q = strchr(p, '.')) != NULL) {
          *q = 0;
          LBS_EmitString(sql, p);
          p = q + 1;
          if (!fDot) {
            LBS_EmitChar(sql, '.');
            fDot = TRUE;
          } else {
            LBS_EmitChar(sql, '_');
          }
        }
        LBS_EmitString(sql, p);
        LBS_EmitSpace(sql);
        break;
      case SQL_OP_INTO:
        n = LBS_FetchInt(src);
        if (n > 0) {
          if (tuple != NULL) {
            xfree(tuple);
          }
          tuple = (ValueStruct **)xmalloc(sizeof(ValueStruct *) * n);
          items = 0;
          fIntoAster = FALSE;
        } else {
          fIntoAster = TRUE;
        }
        break;
      case SQL_OP_STO:
        if (!fIntoAster) {
          if (tuple != NULL) {
            tuple[items] = (ValueStruct *)LBS_FetchPointer(src);
            dbgprintf("STO [%s]", ValueToString(tuple[items], NULL));
          } else {
            Error("tuple is NULL");
          }
          items++;
        }
        break;
      case SQL_OP_REF:
        val = (ValueStruct *)LBS_FetchPointer(src);
        dbgprintf("REF [%s]", ValueToString(val, NULL));
        InsertValues(dbg, sql, val);
        break;
      case SQL_OP_LIMIT:
        dbgprintf("%d", ctrl->limit);
        sprintf(buff, " %d ", ctrl->limit);
        LBS_EmitString(sql, buff);
        break;
      case SQL_OP_VCHAR:
        break;
      case SQL_OP_EOL:
        LBS_EmitEnd(sql);
        res = _PQexec(dbg, LBS_Body(sql));
        LBS_Clear(sql);
        status = PGRES_FATAL_ERROR;
        if ((res == NULL) ||
            ((status = PQresultStatus(res)) == PGRES_BAD_RESPONSE) ||
            (status == PGRES_FATAL_ERROR) || (status == PGRES_NONFATAL_ERROR)) {
          Warning("PostgreSQL: %s: %s: %s: %s", ctrl->func, ctrl->rname,
                  ctrl->pname, PQerrorMessage(PGCONN(dbg)));
          ctrl->rc = -status;
          ctrl->rcount = 0;
          break;
        } else {
          switch (status) {
          case PGRES_TUPLES_OK:
            ctrl->rc = MCP_OK;
            if (fIntoAster) {
              ret = PGresToValue(dbg, ctrl, res, args);
            } else if ((items == 0) || (tuple == NULL)) {
              dbgprintf("items = %d", items);
              dbgmsg("SQL error");
              ctrl->rc = MCP_BAD_SQL;
            } else if ((ntuples = PQntuples(res)) > 0) {
              ctrl->rcount = ntuples;
              if (ntuples == 1) {
                for (j = 0; j < items; j++) {
                  GetValue(dbg, res, 0, j, tuple[j]);
                }
                ret = DuplicateValue(args, TRUE);
              } else {
                ret = NewValue(GL_TYPE_ARRAY);
                for (i = 0; i < ntuples; i++) {
                  for (j = 0; j < items; j++) {
                    GetValue(dbg, res, i, j, tuple[j]);
                  }
                  value = DuplicateValue(args, TRUE);
                  ValueAddArrayItem(ret, i, value);
                }
              }

            } else {
              dbgmsg("EOF");
              ctrl->rc = MCP_EOF;
            }
            if (tuple != NULL) {
              xfree(tuple);
              tuple = NULL;
            }
            break;
          case PGRES_COMMAND_OK:
            ctrl->rcount = cmdTuples(res);
          case PGRES_COPY_OUT:
          case PGRES_COPY_IN:
            dbgmsg("OK");
            ctrl->rc = MCP_OK;
            break;
          case PGRES_EMPTY_QUERY:
          case PGRES_NONFATAL_ERROR:
          default:
            dbgmsg("NONFATAL");
            ctrl->rc = MCP_NONFATAL;
            break;
          }
        }
        _PQclear(res);
        break;
      default:
        dbgprintf("[%X]\n", c);
        break;
      }
    }
  }
  if (tuple != NULL) {
    dbgmsg("exception free");
    xfree(tuple);
  }
  FreeLBS(sql);
  return (ret);
}

static int _EXEC(DBG_Struct *dbg, char *sql) {
  PGresult *res;
  int rc = MCP_OK;

  if (_PQsendQuery(dbg, sql) == TRUE) {
    while ((res = _PQgetResult(dbg)) != NULL) {
      rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
      _PQclear(res);
    }
  } else {
    Warning("PostgreSQL: %s", PQerrorMessage(PGCONN(dbg)));
    rc = MCP_BAD_OTHER;
  }
  return rc;
}

static ValueStruct *_QUERY(DBG_Struct *dbg, char *sql) {
  PGresult *res;
  ValueStruct *ret;
  ExecStatusType status;

  res = _PQexec(dbg, sql);
  if (!CheckConnect(dbg)) {
    ret = NULL;
  } else if ((res == NULL) ||
             ((status = PQresultStatus(res)) == PGRES_BAD_RESPONSE) ||
             (status == PGRES_FATAL_ERROR) ||
             (status == PGRES_NONFATAL_ERROR)) {
    Warning("PostgreSQL: %s:%s", sql, PQerrorMessage(PGCONN(dbg)));
    ret = NULL;
  } else {
    ret = PGresToValue2(dbg, res);
  }
  _PQclear(res);
  return ret;
}

static ValueStruct *_DBOPEN(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  PGconn *conn;
  char *encoding;
  int rc;

  rc = 0;
  if (dbg->dbstatus == DB_STATUS_CONNECT) {
    Warning("database is already connected.");
  }
  if ((conn = PgConnect(dbg)) != NULL) {
    PgInitConnect(conn);
    encoding = GetPGencoding(conn);
    SetDBGcoding(dbg, encoding);
    if (encoding != NULL) {
      xfree(encoding);
    }
    dbg->conn = (void *)conn;
    dbg->dbstatus = DB_STATUS_CONNECT;
    CryptoMode(dbg);
    rc = MCP_OK;
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
    ctrl->rcount = 0;
  }
  return (NULL);
}

static ValueStruct *_DBDISCONNECT(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  if (dbg->dbstatus == DB_STATUS_CONNECT) {
    PQfinish(PGCONN(dbg));
    dbg->dbstatus = DB_STATUS_DISCONNECT;
  }
  if (ctrl != NULL) {
    ctrl->rc = MCP_OK;
    ctrl->rcount = 0;
  }
  return (NULL);
}

static ValueStruct *_DBSTART(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  PGresult *res;
  int rc;

  DbExecTime = 0;
  rc = 0;
  ClearFetchResult();
  if (dbg->dbstatus == DB_STATUS_CONNECT) {
    res = _PQexec(dbg, "BEGIN");
    rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
    _PQclear(res);
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
    ctrl->rcount = 0;
  }
  return (NULL);
}

static ValueStruct *_DBCOMMIT(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  PGresult *res;
  int rc;
  PGconn *conn;
  rc = 0;
  ClearFetchResult();
  if (dbg->dbstatus == DB_STATUS_CONNECT) {
    conn = PGCONN(dbg);
    InTrans(conn);
    res = _PQexec(dbg, "COMMIT WORK");
    rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
    _PQclear(res);
  } else {
    conn = NULL;
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
    ctrl->rcount = 0;
  }
  return (NULL);
}

static ValueStruct *_DBSELECT(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  DB_Struct *db;
  PathStruct *path;
  LargeByteString *src;
  ValueStruct *ret;

  ret = NULL;
  ctrl->rcount = 0;
  ClearFetchResult();
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    ctrl->rc = MCP_OK;
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    src = path->ops[DBOP_SELECT]->proc;
    ret = ExecPGSQL(dbg, ctrl, src, args);
  }
  return (ret);
}

static ValueStruct *_DBFETCH(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                             RecordStruct *rec, ValueStruct *args) {
  char sql[SIZE_SQL + 1];
  DB_Struct *db;
  PathStruct *path;
  ValueStruct *ret;
  LargeByteString *src;

  ret = NULL;
  ctrl->rcount = 0;
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    src = path->ops[DBOP_FETCH]->proc;
    if (src != NULL) {
      ctrl->rc = MCP_OK;
      ret = ExecPGSQL(dbg, ctrl, src, args);
    } else {
      ret = NULL;
      if (fetch_result == NULL) {
        sprintf(sql, "FETCH %d FROM %s_%s_csr", ctrl->limit, ctrl->rname,
              ctrl->pname);
        fetch_result = _PQexec(dbg, sql);
        ctrl->rc = CheckResult(dbg, fetch_result, PGRES_TUPLES_OK);
      }
      ret = PGresToValueFetch(dbg, ctrl, args);
    }
  }
  return (ret);
}

static ValueStruct *_DBCLOSECURSOR(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                   RecordStruct *rec, ValueStruct *args) {
  char sql[SIZE_SQL + 1], *p;
  DB_Struct *db;
  PathStruct *path;
  PGresult *res;
  LargeByteString *src;
  ValueStruct *ret;

  ret = NULL;
  ctrl->rcount = 0;
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    src = path->ops[DBOP_CLOSE]->proc;
    if (src != NULL) {
      ctrl->rc = MCP_OK;
      ret = ExecPGSQL(dbg, ctrl, src, args);
    } else {
      p = sql;
      sprintf(p, "CLOSE %s_%s_csr", ctrl->rname, ctrl->pname);
      res = _PQexec(dbg, sql);
      ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
      _PQclear(res);
    }
  }
  return (ret);
}

static ValueStruct *_DBUPDATE(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  LargeByteString *sql;
  DB_Struct *db;
  char ***item, **pk;
  PGresult *res;
  PathStruct *path;
  LargeByteString *src;
  ValueStruct *ret;

  ret = NULL;
  ctrl->rcount = 0;
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    src = path->ops[DBOP_UPDATE]->proc;
    if (src != NULL) {
      ctrl->rc = MCP_OK;
      ret = ExecPGSQL(dbg, ctrl, src, args);
    } else {
      sql = NewLBS();
      LBS_EmitString(sql, "UPDATE ");
      LBS_EmitString(sql, rec->name);
      LBS_EmitString(sql, "\tSET ");
      level = 0;
      alevel = 0;
      UpdateValue(dbg, sql, args);

      LBS_EmitString(sql, "WHERE\t");
      item = db->pkey->item;
      while (*item != NULL) {
        LBS_EmitString(sql, rec->name);
        LBS_EmitChar(sql, '.');
        pk = *item;
        while (*pk != NULL) {
          LBS_EmitString(sql, *pk);
          pk++;
          if (*pk != NULL) {
            LBS_EmitChar(sql, '_');
          }
        }
        LBS_EmitString(sql, " = ");
        KeyValue(dbg, sql, args, *item);
        LBS_EmitChar(sql, ' ');
        item++;
        if (*item != NULL) {
          LBS_EmitString(sql, "AND\t");
        }
      }
      LBS_EmitEnd(sql);
      res = _PQexec(dbg, LBS_Body(sql));
      ctrl->rcount = cmdTuples(res);
      ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
      _PQclear(res);
      FreeLBS(sql);
    }
  }
  return (ret);
}

static ValueStruct *_DBDELETE(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  LargeByteString *sql;
  DB_Struct *db;
  char ***item, **pk;
  PGresult *res;
  PathStruct *path;
  LargeByteString *src;
  ValueStruct *ret;

  ret = NULL;
  ctrl->rcount = 0;
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    src = path->ops[DBOP_DELETE]->proc;
    if (src != NULL) {
      ctrl->rc = MCP_OK;
      ret = ExecPGSQL(dbg, ctrl, src, args);
    } else {
      sql = NewLBS();
      LBS_EmitString(sql, "DELETE\tFROM\t");
      LBS_EmitString(sql, rec->name);
      LBS_EmitString(sql, " WHERE\t");
      item = db->pkey->item;
      while (*item != NULL) {
        pk = *item;
        LBS_EmitString(sql, rec->name);
        LBS_EmitChar(sql, '.');
        while (*pk != NULL) {
          LBS_EmitString(sql, *pk);
          pk++;
          if (*pk != NULL) {
            LBS_EmitChar(sql, ' ');
          }
        }
        LBS_EmitString(sql, " = ");
        KeyValue(dbg, sql, args, *item);
        LBS_EmitChar(sql, ' ');
        item++;
        if (*item != NULL) {
          LBS_EmitString(sql, "AND\t");
        }
      }
      LBS_EmitEnd(sql);
      res = _PQexec(dbg, LBS_Body(sql));
      ctrl->rcount = cmdTuples(res);
      ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
      _PQclear(res);
      FreeLBS(sql);
    }
  }
  return (ret);
}

static ValueStruct *_DBINSERT(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  LargeByteString *sql;
  DB_Struct *db;
  PGresult *res;
  PathStruct *path;
  LargeByteString *src;
  ValueStruct *ret;

  ret = NULL;
  ctrl->rcount = 0;
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    src = path->ops[DBOP_INSERT]->proc;
    if (src != NULL) {
      ctrl->rc = MCP_OK;
      ret = ExecPGSQL(dbg, ctrl, src, args);
    } else {
      sql = NewLBS();
      LBS_EmitString(sql, "INSERT\tINTO\t");
      LBS_EmitString(sql, rec->name);
      LBS_EmitString(sql, " (");
      level = 0;
      alevel = 0;
      InsertNames(sql, args);
      LBS_EmitString(sql, ") VALUES\t(");
      InsertValues(dbg, sql, args);
      LBS_EmitString(sql, ") ");
      LBS_EmitEnd(sql);
      res = _PQexec(dbg, LBS_Body(sql));
      ctrl->rcount = cmdTuples(res);
      ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
      _PQclear(res);
      FreeLBS(sql);
    }
  }
  return (ret);
}

static ValueStruct *_DBESCAPE(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  LargeByteString *lbs;
  ValueStruct *ret;
  ValueStruct *val;
  if ((val = GetItemLongName(args, "dbescapestring")) == NULL) {
    Warning("dbescapestring is not found.");
    return DuplicateValue(args, TRUE);
  }

  lbs = NewLBS();
  EscapeString(dbg, lbs, ValueToString(val, dbg->coding));
  LBS_EmitEnd(lbs);
  SetValueString(val, LBS_Body(lbs), dbg->coding);
  FreeLBS(lbs);
  ret = DuplicateValue(args, TRUE);
  return (ret);
}

static ValueStruct *_DBESCAPEBYTEA(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                   RecordStruct *rec, ValueStruct *args) {
  LargeByteString *lbs;
  ValueStruct *ret, *bytea;
  ValueStruct *val;
  if (IS_VALUE_RECORD(args)) {
    if ((val = GetItemLongName(args, "dbescapebytea")) == NULL) {
      Warning("dbescapebytea is not found.");
      return args;
    }
    ret = NewValue(GL_TYPE_RECORD);
    lbs = NewLBS();
    EscapeBytea(dbg, lbs, ValueByte(val), ValueByteLength(val));
    LBS_EmitEnd(lbs);
    bytea = NewValue(GL_TYPE_TEXT);
    SetValueString(bytea, LBS_Body(lbs), dbg->coding);
    ValueAddRecordItem(ret, "dbescapebytea", bytea);
    FreeLBS(lbs);
  } else {
    lbs = NewLBS();
    EscapeBytea(dbg, lbs, ValueByte(args), ValueByteLength(args));
    LBS_EmitEnd(lbs);
    ret = NewValue(GL_TYPE_TEXT);
    SetValueString(ret, LBS_Body(lbs), dbg->coding);
    FreeLBS(lbs);
  }
  return ret;
}

static ValueStruct *_DBUNESCAPEBYTEA(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                     RecordStruct *rec, ValueStruct *args) {
  ValueStruct *bin, *val, *ret;
  if (IS_VALUE_RECORD(args)) {
    if ((val = GetItemLongName(args, "dbunescapebytea")) == NULL) {
      Warning("dbunescapebytea is not found.");
      return args;
    }
    bin = UnEscapeBytea(ValueStringPointer(val));
    ValueAddRecordItem(args, "dbunescapebytea", bin);
    ret = args;
  } else {
    ret = UnEscapeBytea(ValueStringPointer(args));
  }
  return ret;
}

static ValueStruct *_DBLOCK(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                            RecordStruct *rec, ValueStruct *args) {
  LargeByteString *sql;
  PGresult *res;
  ValueStruct *ret;
  ret = NULL;
  ctrl->rcount = 0;
  if (rec == NULL) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    sql = NewLBS();
    LBS_EmitString(sql, "LOCK\tTABLE\t");
    LBS_EmitString(sql, rec->name);
    LBS_EmitEnd(sql);
    res = _PQexec(dbg, LBS_Body(sql));
    ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
    _PQclear(res);
    FreeLBS(sql);
  }
  return (ret);
}

static ValueStruct *_DBACCESS(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  DB_Struct *db;
  PathStruct *path;
  LargeByteString *src;
  int ix;
  ValueStruct *ret;

  ret = NULL;
  ctrl->rcount = 0;
  dbgprintf("[%s]", ctrl->func);
  if ((rec == NULL) || (rec->type != RECORD_DB)) {
    ctrl->rc = MCP_BAD_ARG;
  } else {
    db = rec->opt.db;
    path = db->path[ctrl->pno];
    if ((ix = (int)(long)g_hash_table_lookup(path->opHash, ctrl->func)) == 0) {
      ctrl->rc = MCP_BAD_FUNC;
    } else {
      src = path->ops[ix - 1]->proc;
      if (src != NULL) {
        ctrl->rc = MCP_OK;
        ret = ExecPGSQL(dbg, ctrl, src, args);
      } else {
        ctrl->rc = MCP_BAD_OTHER;
      }
    }
  }
  return (ret);
}

static DB_OPS Operations[] = {
    /*	DB operations		*/
    {"DBOPEN", (DB_FUNC)_DBOPEN},
    {"DBDISCONNECT", (DB_FUNC)_DBDISCONNECT},
    {"DBSTART", (DB_FUNC)_DBSTART},
    {"DBCOMMIT", (DB_FUNC)_DBCOMMIT},
    /*	table operations	*/
    {"DBSELECT", _DBSELECT},
    {"DBFETCH", _DBFETCH},
    {"DBUPDATE", _DBUPDATE},
    {"DBDELETE", _DBDELETE},
    {"DBINSERT", _DBINSERT},
    {"DBCLOSECURSOR", _DBCLOSECURSOR},
    {"DBESCAPE", _DBESCAPE},
    {"DBESCAPEBYTEA", _DBESCAPEBYTEA},
    {"DBUNESCAPEBYTEA", _DBUNESCAPEBYTEA},
    {"DBLOCK", _DBLOCK},

    {NULL, NULL}};

static DB_Primitives Core = {
    _EXEC,
    _DBACCESS,
    _QUERY,
    NULL,
};

extern DB_Func *InitPostgreSQL(void) {
  return (EnterDB_Function("PostgreSQL", Operations, DB_PARSER_SQL, &Core, "/*",
                           "*/\t"));
}

#endif /* #ifdef HAVE_POSTGRES */
