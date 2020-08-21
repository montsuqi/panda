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

#define MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <glib.h>
#include "libmondai.h"
#include "RecParser.h"
#include "struct.h"
#include "const.h"
#include "enum.h"
#include "wfcdata.h"
#include "DBparser.h"
#include "directory.h"
#include "dbgroup.h"
#include "dirs.h"
#include "option.h"
#include "message.h"
#include "debug.h"

static Bool fNoConv;
static Bool fNoFiller;
static Bool fFiller;
static Bool fSPA;
static Bool fLinkage;
static Bool fScreen;
static Bool fWindowPrefix;
static Bool fRecordPrefix;
static Bool fLDW;
static Bool fLDR;
static Bool fDB;
static Bool fDBREC;
static Bool fDBCOMM;
static Bool fDBPATH;
static Bool fMCP;
static Bool fFull;
static int TextSize;
static int ArraySize;
static char *Prefix;
static char *RecName;
static char *LD_Name;
static char *BD_Name;
static char *Directory;
static char *Lang;
static int Top;

static CONVOPT *Conv;

static int level;
static int Col;
static int is_return = FALSE;
static char namebuff[SIZE_BUFF];

static void _COBOL(CONVOPT *conv, ValueStruct *val);

static void PutLevel(int level, Bool fNum) {
  int n;

  n = 8;
  if (level > 1) {
    n += 4 + (level - 2) * 2;
  }
  for (; n > 0; n--) {
    fputc(' ', stdout);
  }
  if (fNum) {
    printf("%02d  ", level);
  } else {
    printf("    ");
  }
  Col = 0;
}

static void PutTab(int unit) {
  int n;

  n = unit - (Col % unit);
  Col += n;
  for (; n > 0; n--) {
    fputc(' ', stdout);
  }
}

static void PutChar(int c) {
  fputc(c, stdout);
  Col++;
}

static void PutString(char *str) {
  int c;

  while (*str != 0) {
    if (fNoConv) {
      PutChar(*str);
    } else {
      c = toupper(*str);
      if (c == '_') {
        c = '-';
      }
      PutChar(c);
    }
    str++;
  }
}

static void PutName(char *name) {
  char buff[SIZE_BUFF];

  if ((name == NULL) || (!stricmp(name, "filler"))) {
    strcpy(buff, "filler");
  } else {
    int threshold;

    sprintf(buff, "%s%s", Prefix, name);
#define MAX_CHARACTER_IN_LINE 65

    threshold =
        MAX_CHARACTER_IN_LINE - (8 + level * 2 + 4 + 4 + 18 + 2 + 18 + 1);
    if ((threshold <= 0) || (strlen(buff) > threshold)) {
      is_return = TRUE;
    }
  }
  PutString(buff);
}

static int PrevCount;

static void SubItem(char *name, char *buff) {
  PutLevel(level + 1, TRUE);
  PrevCount = 0;
  PutName(name);
  PutTab(4);
  PutString(buff);
}

static char PrevName[SIZE_BUFF];
static char *PrevSuffix[] = {"X", "Y", "Z"};
static void _COBOL(CONVOPT *conv, ValueStruct *val) {
  int i, n;
  ValueStruct *tmp, *dummy;
  char buff[SIZE_BUFF + 1];
  char *name;

  if (val == NULL)
    return;

  switch (ValueType(val)) {
  case GL_TYPE_INT:
    PutString("PIC S9(9)   BINARY");
    break;
  case GL_TYPE_FLOAT:
    PutString("PIC X(8)");
    break;
  case GL_TYPE_BOOL:
    PutString("PIC X");
    break;
  case GL_TYPE_BYTE:
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_DBCODE:
    sprintf(buff, "PIC X(%d)", (int)ValueStringLength(val));
    PutString(buff);
    break;
  case GL_TYPE_OBJECT:
    sprintf(buff, "PIC X(%d)", SIZE_UUID);
    PutString(buff);
    break;
  case GL_TYPE_NUMBER:
    if (ValueFixedSlen(val) == 0) {
      sprintf(buff, "PIC S9(%d)", (int)ValueFixedLength(val));
    } else {
      sprintf(buff, "PIC S9(%d)V9(%d)",
              (int)(ValueFixedLength(val) - ValueFixedSlen(val)),
              (int)ValueFixedSlen(val));
    }
    PutString(buff);
    break;
  case GL_TYPE_TEXT:
    sprintf(buff, "PIC X(%d)", (int)conv->textsize);
    PutString(buff);
    break;
  case GL_TYPE_TIMESTAMP:
    printf(".\n");
    if (fFull) {
      sprintf(buff, "%s-YEAR", namebuff);
    } else {
      sprintf(buff, "%s-YEAR", PrevName);
    }
    SubItem(buff, "PIC 9(4).\n");
    if (fFull) {
      sprintf(buff, "%s-MONTH", namebuff);
    } else {
      sprintf(buff, "%s-MONTH", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-MDAY", namebuff);
    } else {
      sprintf(buff, "%s-MDAY", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-HOUR", namebuff);
    } else {
      sprintf(buff, "%s-HOUR", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-MIN ", namebuff);
    } else {
      sprintf(buff, "%s-MIN ", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-SEC ", namebuff);
    } else {
      sprintf(buff, "%s-SEC ", PrevName);
    }
    SubItem(buff, "PIC 9(2)");
    break;
  case GL_TYPE_TIME:
    printf(".\n");
    if (fFull) {
      sprintf(buff, "%s-HOUR", namebuff);
    } else {
      sprintf(buff, "%s-HOUR", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-MIN ", namebuff);
    } else {
      sprintf(buff, "%s-MIN ", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-SEC ", namebuff);
    } else {
      sprintf(buff, "%s-SEC ", PrevName);
    }
    SubItem(buff, "PIC 9(2)");
    break;
  case GL_TYPE_DATE:
    printf(".\n");
    if (fFull) {
      sprintf(buff, "%s-YEAR", namebuff);
    } else {
      sprintf(buff, "%s-YEAR", PrevName);
    }
    SubItem(buff, "PIC 9(4).\n");
    if (fFull) {
      sprintf(buff, "%s-MONTH", namebuff);
    } else {
      sprintf(buff, "%s-MONTH", PrevName);
    }
    SubItem(buff, "PIC 9(2).\n");
    if (fFull) {
      sprintf(buff, "%s-MDAY", namebuff);
    } else {
      sprintf(buff, "%s-MDAY", PrevName);
    }
    SubItem(buff, "PIC 9(2)");
    break;
  case GL_TYPE_ARRAY:
    tmp = ValueArrayItem(val, 0);
    n = ValueArraySize(val);
    if (n == 0) {
      n = conv->arraysize;
    }
    switch (ValueType(tmp)) {
    case GL_TYPE_ROOT_RECORD:
    case GL_TYPE_RECORD:
      sprintf(buff, "OCCURS  %d TIMES", n);
      PutTab(8);
      PutString(buff);
      _COBOL(conv, tmp);
      break;
    case GL_TYPE_ARRAY:
      sprintf(buff, "OCCURS  %d TIMES", n);
      PutTab(8);
      PutString(buff);
      dummy = NewValue(GL_TYPE_RECORD);
      sprintf(buff, "%s-%s", PrevName, PrevSuffix[PrevCount]);
      ValueAddRecordItem(dummy, buff, tmp);
      PrevCount++;
      _COBOL(conv, dummy);
      break;
    default:
      _COBOL(conv, tmp);
      sprintf(buff, "OCCURS  %d TIMES", n);
      PutTab(8);
      PutString(buff);
      break;
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    level++;
    if (fFull) {
      name = namebuff + strlen(namebuff);
    } else {
      name = namebuff;
    }
    for (i = 0; i < ValueRecordSize(val); i++) {
      printf(".\n");
      PutLevel(level, TRUE);
      if (fFull) {
        sprintf(name, "%s%s", ((*namebuff == 0) ? "" : "-"),
                ValueRecordName(val, i));
      } else {
        strcpy(namebuff, ValueRecordName(val, i));
      }
      strcpy(PrevName, ValueRecordName(val, i));
      PrevCount = 0;
      PutName(namebuff);
      tmp = ValueRecordItem(val, i);
      if (is_return) {
        /* new line if current ValueStruct children is item and it has
           occurs */
        if (!IS_VALUE_RECORD(tmp) && IS_VALUE_ARRAY(tmp) &&
            !IS_VALUE_RECORD(ValueArrayItem(tmp, 0))) {
          printf("\n");
          PutLevel(level, FALSE);
        }
        is_return = FALSE;
      }
      if (!IS_VALUE_RECORD(tmp)) {
        PutTab(4);
      }
      _COBOL(conv, tmp);
      *name = 0;
    }
    level--;
    break;
  case GL_TYPE_ALIAS:
  default:
    break;
  }
}

static void COBOL(CONVOPT *conv, ValueStruct *val) {
  *namebuff = 0;
  _COBOL(conv, val);
}

static void SIZE(CONVOPT *conv, ValueStruct *val) {
  char buff[SIZE_BUFF + 1];

  if (val == NULL)
    return;
  level++;
  PutLevel(level, TRUE);
  PutName("filler");
  PutTab(8);
  sprintf(buff, "PIC X(%d)", (int)SizeValue(conv, val));
  PutString(buff);
  level--;
}

static void MakeFromRecord(char *name) {
  RecordStruct *rec;

  level = Top;
  RecParserInit();
  DB_ParserInit();
  if ((rec = DB_Parser(name, NULL, FALSE)) != NULL) {
    PutLevel(level, TRUE);
    if (*RecName == 0) {
      PutString(rec->name);
    } else {
      PutString(RecName);
    }
    if (fFiller) {
      printf(".\n");
      SIZE(Conv, rec->value);
    } else {
      COBOL(Conv, rec->value);
    }
    printf(".\n");
  }
}

static void MakeLD(void) {
  LD_Struct *ld;
  int i;
  char buff[SIZE_BUFF + 1];
  char *_prefix;
  size_t size, num, spasize, linksize;
  int base;

  InitDirectory();
  SetUpDirectory(Directory, LD_Name, "", "", P_LD);
  if ((ld = GetLD(LD_Name)) == NULL) {
    Error("LD not found.\n");
  }

  PutLevel(Top, TRUE);
  if (*RecName == 0) {
    sprintf(buff, "%s", ld->name);
  } else {
    sprintf(buff, "%s", RecName);
  }
  PutString(buff);
  printf(".\n");

  ConvSetSize(Conv, ld->textsize, ld->arraysize);

  base = Top + 1;
  if ((fLDR) || (fLDW)) {
    size = SizeValue(Conv, ThisEnv->mcprec->value) +
           SizeValue(Conv, ThisEnv->linkrec->value) +
           SizeValue(Conv, ld->sparec->value);
    for (i = 0; i < ld->cWindow; i++) {
      size += SizeValue(Conv, ld->windows[i]->value);
    }
    num = (size / SIZE_BLOCK) + 1;

    PutLevel(base, TRUE);
    PutName("dataarea");
    printf(".\n");

    PutLevel(base + 1, TRUE);
    PutName("data");
    PutTab(12);
    printf("OCCURS  %d.\n", (int)num);

    PutLevel(base + 2, TRUE);
    PutName("filler");
    PutTab(12);
    printf("PIC X(%d).\n", SIZE_BLOCK);

    PutLevel(base, TRUE);
    PutName("inputarea-red");
    PutTab(4);
    printf("REDEFINES   ");
    PutName("dataarea.\n");
    base++;
  } else {
    num = 0;
  }
  if (fMCP) {
    if (SizeValue(Conv, ThisEnv->mcprec->value) > 0) {
      PutLevel(base, TRUE);
      PutName("mcpdata");
      level = base;
      if ((fFiller) || (fLDW)) {
        printf(".\n");
        SIZE(Conv, ThisEnv->mcprec->value);
      } else {
        _prefix = Prefix;
        Prefix = "ldr-mcp-";
        COBOL(Conv, ThisEnv->mcprec->value);
        Prefix = _prefix;
      }
      printf(".\n");
    }
  }
  if (fSPA) {
    if ((spasize = SizeValue(Conv, ld->sparec->value)) > 0) {
      PutLevel(base, TRUE);
      PutName("spadata");
      if (ld->sparec == NULL) {
        printf(".\n");
        PutLevel(base + 1, TRUE);
        PutName("filler");
        printf("      PIC X(%d).\n", (int)spasize);
      } else {
        level = base;
        if ((fFiller) || (fLDW)) {
          printf(".\n");
          SIZE(Conv, ld->sparec->value);
        } else {
          _prefix = Prefix;
          Prefix = "spa-";
          COBOL(Conv, ld->sparec->value);
          Prefix = _prefix;
        }
        printf(".\n");
      }
    }
  }
  if (fLinkage) {
    if ((linksize = SizeValue(Conv, ThisEnv->linkrec->value)) > 0) {
      PutLevel(base, TRUE);
      PutName("linkdata");
      level = base;
      if ((fFiller) || (fLDR) || (fLDW)) {
        printf(".\n");
        PutLevel(level + 1, TRUE);
        PutName("filler");
        printf("      PIC X(%d)", (int)linksize);
      } else {
        _prefix = Prefix;
        Prefix = "lnk-";
        COBOL(Conv, ThisEnv->linkrec->value);
        Prefix = _prefix;
      }
      printf(".\n");
    }
  }
  if (fScreen) {
    PutLevel(base, TRUE);
    sprintf(buff, "screendata");
    PutName(buff);
    printf(".\n");
    _prefix = Prefix;
    for (i = 0; i < ld->cWindow; i++) {
      if (SizeValue(Conv, ld->windows[i]->value) > 0) {
        Prefix = _prefix;
        PutLevel(base + 1, TRUE);
        sprintf(buff, "%s", ld->windows[i]->name);
        PutName(buff);
        if (fWindowPrefix) {
          sprintf(buff, "%s-", ld->windows[i]->name);
          Prefix = StrDup(buff);
        }
        level = base + 1;
        if ((fFiller) || (fLDR) || (fLDW)) {
          printf(".\n");
          SIZE(Conv, ld->windows[i]->value);
        } else {
          COBOL(Conv, ld->windows[i]->value);
        }
        printf(".\n");
        if (fWindowPrefix) {
          xfree(Prefix);
        }
      }
    }
  }
  if ((fLDR) || (fLDW)) {
    PutLevel(Top, TRUE);
    PutName("blocks");
    PutTab(8);
    printf("PIC S9(9)   BINARY  VALUE   %d.\n", (int)num);
  }
}

static void MakeLinkage(void) {
  LD_Struct *ld;
  char *_prefix;
  size_t size;

  InitDirectory();
  SetUpDirectory(Directory, LD_Name, "", "", P_LD);
  if ((ld = GetLD(LD_Name)) == NULL) {
    Error("LD not found.\n");
  }

  ConvSetSize(Conv, ld->textsize, ld->arraysize);

#if 1
  size = SizeValue(Conv, ThisEnv->linkrec->value);
#else
  if (ThisEnv->linksize <= 0) {
    if ((ThisEnv->linkrec != NULL) && (ThisEnv->linkrec->value != NULL)) {
      size = SizeValue(Conv, ThisEnv->linkrec->value);
    } else {
      Error("linkarea not found.\n");
    }
  } else {
    size = ThisEnv->linksize;
  }
#endif
  _prefix = Prefix;
  Prefix = "";
  PutLevel(Top, TRUE);
  PutName("linkarea.\n");
  PutLevel(Top + 1, TRUE);
  PutName("linkdata-redefine.\n");
  PutLevel(Top + 2, TRUE);
  PutName("filler");
  printf("      PIC X(%d).\n", (int)size);

  PutLevel(Top + 1, TRUE);
  PutName(ld->name);
  PutTab(4);
  printf("REDEFINES   ");
  PutName("linkdata-redefine");
  Prefix = _prefix;
  level = Top + 2;
  COBOL(Conv, ThisEnv->linkrec->value);
  printf(".\n");
}

static void MakeDB(void) {
  size_t msize, size;
  int i;
  LD_Struct *ld;
  BD_Struct *bd;
  RecordStruct **dbrec;
  size_t arraysize, textsize;
  size_t cDB;

  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_ALL);
  if (LD_Name != NULL) {
    if ((ld = GetLD(LD_Name)) == NULL) {
      Error("LD not found.\n");
    }
    dbrec = ld->db;
    arraysize = ld->arraysize;
    textsize = ld->textsize;
    cDB = ld->cDB;
  } else if (BD_Name != NULL) {
    if ((bd = GetBD(BD_Name)) == NULL) {
      Error("BD not found.\n");
    }
    dbrec = bd->db;
    arraysize = bd->arraysize;
    textsize = bd->textsize;
    cDB = bd->cDB;
  } else {
    Error("LD or BD not specified");
    exit(1);
  }

  ConvSetSize(Conv, textsize, arraysize);

  PutLevel(Top, TRUE);
  PutName("dbarea");
  printf(".\n");
  msize = 0;
  for (i = 1; i < cDB; i++) {
    size = SizeValue(Conv, dbrec[i]->value);
    msize = (msize > size) ? msize : size;
  }

  PutLevel(Top + 1, TRUE);
  PutName("dbdata");
  PutTab(12);
  printf("PIC X(%d).\n", (int)msize);
}

static void PutDBREC(ValueStruct *value, char *rname, size_t msize) {
  size_t size;
  char *_prefix;
  char prefix[SIZE_LONGNAME + 1];

  level = Top;
  PutLevel(level, TRUE);
  if (fRecordPrefix) {
    sprintf(prefix, "%s-", rname);
    Prefix = prefix;
  }
  _prefix = Prefix;
  Prefix = "";
  PutName(rname);
  Prefix = _prefix;
  COBOL(Conv, value);
  printf(".\n");

  if (!fNoFiller) {
    size = SizeValue(Conv, value);
    if (msize != size) {
      PutLevel(Top + 1, TRUE);
      PutName("filler");
      PutTab(12);
      printf("PIC X(%d).\n", (int)(msize - size));
    }
  }
}

static void MakeDBREC(char *name) {
  size_t msize, size;
  int i, j, k;
  char rname[SIZE_LONGNAME + 1];
  LD_Struct *ld;
  BD_Struct *bd;
  RecordStruct **dbrec, *rec;
  PathStruct *path;
  DB_Operation *op;
  GHashTable *dbtable;
  size_t arraysize, textsize;
  size_t cDB;
  char *p;
  int rno;

  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_ALL);
  if (LD_Name != NULL) {
    if ((ld = GetLD(LD_Name)) == NULL) {
      Error("LD not found.\n");
    }
    dbrec = ld->db;
    arraysize = ld->arraysize;
    textsize = ld->textsize;
    cDB = ld->cDB;
    dbtable = ld->DB_Table;
  } else if (BD_Name != NULL) {
    if ((bd = GetBD(BD_Name)) == NULL) {
      Error("BD not found.\n");
    }
    dbrec = bd->db;
    arraysize = bd->arraysize;
    textsize = bd->textsize;
    cDB = bd->cDB;
    dbtable = bd->DB_Table;
  } else {
    Error("LD or BD not specified");
    exit(1);
  }

  ConvSetSize(Conv, textsize, arraysize);
  msize = 64;
  for (i = 1; i < cDB; i++) {
    rec = dbrec[i];
    size = SizeValue(Conv, rec->value);
    msize = (msize > size) ? msize : size;
    for (j = 0; j < rec->opt.db->pcount; j++) {
      path = rec->opt.db->path[j];
      if (path->args != NULL) {
        size = SizeValue(Conv, path->args);
        msize = (msize > size) ? msize : size;
      }
      for (k = 0; k < path->ocount; k++) {
        op = path->ops[k];
        if (op->args != NULL) {
          size = SizeValue(Conv, op->args);
          msize = (msize > size) ? msize : size;
        }
      }
    }
  }
  if ((p = strchr(name, '.')) != NULL) {
    *p = 0;
  }
  if ((rno = (int)(long)g_hash_table_lookup(dbtable, name)) != 0) {
    rec = dbrec[rno - 1];
    if (*RecName == 0) {
      strcpy(rname, rec->name);
    } else {
      strcpy(rname, RecName);
    }
    PutDBREC(rec->value, rname, msize);
    for (j = 0; j < rec->opt.db->pcount; j++) {
      path = rec->opt.db->path[j];
      if (path->args != NULL) {
        if (*RecName == 0) {
          sprintf(rname, "%s-%s", rec->name, path->name);
        } else {
          sprintf(rname, "%s-%s", RecName, path->name);
        }
        PutDBREC(path->args, rname, msize);
      }
      for (k = 0; k < path->ocount; k++) {
        op = path->ops[k];
        if (op->args != NULL) {
          if (*RecName == 0) {
            sprintf(rname, "%s-%s-%s", rec->name, path->name, op->name);
          } else {
            sprintf(rname, "%s-%s-%s", RecName, path->name, op->name);
          }
          PutDBREC(op->args, rname, msize);
        }
      }
    }
  }
}

static void MakeDBCOMM(void) {
  size_t msize, size, num;
  int i;
  LD_Struct *ld;
  BD_Struct *bd;
  RecordStruct **dbrec;
  size_t arraysize, textsize;
  size_t cDB;

  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_ALL);
  if (LD_Name != NULL) {
    if ((ld = GetLD(LD_Name)) == NULL) {
      Error("LD not found.\n");
    }
    dbrec = ld->db;
    arraysize = ld->arraysize;
    textsize = ld->textsize;
    cDB = ld->cDB;
  } else if (BD_Name != NULL) {
    if ((bd = GetBD(BD_Name)) == NULL) {
      Error("BD not found.\n");
    }
    dbrec = bd->db;
    arraysize = bd->arraysize;
    textsize = bd->textsize;
    cDB = bd->cDB;
  } else {
    Error("LD or BD not specified");
    exit(1);
  }

  ConvSetSize(Conv, textsize, arraysize);

  msize = 0;
  for (i = 1; i < cDB; i++) {
    size = SizeValue(Conv, dbrec[i]->value);
    msize = (msize > size) ? msize : size;
  }

  PutLevel(Top, TRUE);
  PutName("dbcomm");
  printf(".\n");

  num = ((msize + sizeof(DBCOMM_CTRL)) / SIZE_BLOCK) + 1;

  PutLevel(Top + 1, TRUE);
  PutName("dbcomm-blocks");
  printf(".\n");

  PutLevel(Top + 2, TRUE);
  PutName("dbcomm-block");
  PutTab(12);
  printf("OCCURS  %d.\n", (int)num);

  PutLevel(Top + 3, TRUE);
  PutName("filler");
  PutTab(12);
  printf("PIC X(%d).\n", SIZE_BLOCK);

  PutLevel(Top + 1, TRUE);
  PutName("dbcomm-data");
  PutTab(4);
  printf("REDEFINES   ");
  PutName("dbcomm-blocks");
  printf(".\n");

  PutLevel(Top + 2, TRUE);
  PutName("dbcomm-ctrl");
  printf(".\n");

  PutLevel(Top + 3, TRUE);
  PutName("dbcomm-func");
  PutTab(12);
  printf("PIC X(16).\n");

  PutLevel(Top + 3, TRUE);
  PutName("dbcomm-rc");
  PutTab(12);
  printf("PIC S9(9)   BINARY.\n");

  PutLevel(Top + 3, TRUE);
  PutName("dbcomm-path");
  printf(".\n");

  PutLevel(Top + 4, TRUE);
  PutName("dbcomm-path-blocks");
  PutTab(12);
  printf("PIC S9(9)   BINARY.\n");

  PutLevel(Top + 4, TRUE);
  PutName("dbcomm-path-rname");
  PutTab(12);
  printf("PIC S9(9)   BINARY.\n");

  PutLevel(Top + 4, TRUE);
  PutName("dbcomm-path-pname");
  PutTab(12);
  printf("PIC S9(9)   BINARY.\n");

  PutLevel(Top + 2, TRUE);
  PutName("dbcomm-record");
  printf(".\n");

  PutLevel(Top + 3, TRUE);
  PutName("filler");
  PutTab(12);
  printf("PIC X(%d).\n", (int)msize);
}

static void MakeDBPATH(void) {
  int i, j, blocks;
  size_t size;
  char buff[SIZE_NAME * 2 + 1];
  DB_Struct *db;
  LD_Struct *ld;
  BD_Struct *bd;
  RecordStruct **dbrec;
  size_t arraysize, textsize;
  size_t cDB;

  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_ALL);
  if (LD_Name != NULL) {
    if ((ld = GetLD(LD_Name)) == NULL) {
      Error("LD not found.\n");
      exit(1);
    } else {
      dbrec = ld->db;
      arraysize = ld->arraysize;
      textsize = ld->textsize;
      cDB = ld->cDB;
    }
  } else if (BD_Name != NULL) {
    if ((bd = GetBD(BD_Name)) == NULL) {
      Error("BD not found.\n");
      exit(1);
    } else {
      dbrec = bd->db;
      arraysize = bd->arraysize;
      textsize = bd->textsize;
      cDB = bd->cDB;
    }
  } else {
    Error("LD or BD not specified");
    exit(1);
  }

  ConvSetSize(Conv, textsize, arraysize);

  PutLevel(Top, TRUE);
  PutName("dbpath");
  printf(".\n");
  for (i = 1; i < cDB; i++) {
    db = dbrec[i]->opt.db;
    size = SizeValue(Conv, dbrec[i]->value);
    blocks = ((size + sizeof(DBCOMM_CTRL)) / SIZE_BLOCK) + 1;

    for (j = 0; j < db->pcount; j++) {
      PutLevel(Top + 1, TRUE);
      sprintf(buff, "path-%s-%s", dbrec[i]->name, db->path[j]->name);
      PutName(buff);
      printf(".\n");

      PutLevel(Top + 2, TRUE);
      PutName("filler");
      PutTab(12);
      printf("PIC S9(9)   BINARY  VALUE %d.\n", blocks);

      PutLevel(Top + 2, TRUE);
      PutName("filler");
      PutTab(12);
      printf("PIC S9(9)   BINARY  VALUE %d.\n", i);

      PutLevel(Top + 2, TRUE);
      PutName("filler");
      PutTab(12);
      printf("PIC S9(9)   BINARY  VALUE %d.\n", j);
    }
  }
}

static void MakeMCPAREA(void) {
  InitDirectory();
  SetUpDirectory(Directory, "", "", "", P_NONE);

  Prefix = "";
  PutLevel(Top, TRUE);
  PutName("mcparea");
  Prefix = "mcp_";
  level = Top;
  COBOL(Conv, ThisEnv->mcprec->value);
  printf(".\n");
}

static ARG_TABLE option[] = {
    {"top", INTEGER, TRUE, (void *)&Top, "top level number"},
    {"ldw", BOOLEAN, TRUE, (void *)&fLDW,
     "制御フィールド以外をFILLERにする(dotCOBOL)"},
    {"ldr", BOOLEAN, TRUE, (void *)&fLDR,
     "制御フィールドとSPA以外をFILLERにする(dotCOBOL)"},
    {"spa", BOOLEAN, TRUE, (void *)&fSPA, "SPA領域を出力する(dotCOBOL)"},
    {"linkage", BOOLEAN, TRUE, (void *)&fLinkage, "連絡領域を出力する"},
    {"screen", BOOLEAN, TRUE, (void *)&fScreen, "画面レコード領域を出力する"},
    {"dbarea", BOOLEAN, TRUE, (void *)&fDB, "MCPSUB用領域を出力する"},
    {"dbrec", BOOLEAN, TRUE, (void *)&fDBREC,
     "MCBSUBの引数に使うレコード領域を出力する"},
    {"dbpath", BOOLEAN, TRUE, (void *)&fDBPATH, "DBのパス名テーブルを出力する"},
    {"dbcomm", BOOLEAN, TRUE, (void *)&fDBCOMM,
     "MCPSUBとDBサーバとの通信領域を出力する"},
    {"mcp", BOOLEAN, TRUE, (void *)&fMCP, "MCPAREAを出力する"},
    {"lang", STRING, TRUE, (void *)&Lang, "対象言語名"},
    {"textsize", INTEGER, TRUE, (void *)&TextSize, "textの最大長"},
    {"arraysize", INTEGER, TRUE, (void *)&ArraySize,
     "可変要素配列の最大繰り返し数"},
    {"prefix", STRING, TRUE, (void *)&Prefix, "項目名の前に付加する文字列"},
    {"wprefix", BOOLEAN, TRUE, (void *)&fWindowPrefix,
     "画面レコードの項目の前にウィンドウ名を付加する"},
    {"rprefix", BOOLEAN, TRUE, (void *)&fRecordPrefix,
     "データベースレコードの項目の前にレコード名を付加する"},
    {"name", STRING, TRUE, (void *)&RecName, "レコードの名前"},
    {"filler", BOOLEAN, TRUE, (void *)&fFiller, "レコードの中身をFILLERにする"},
    {"full", BOOLEAN, TRUE, (void *)&fFull, "階層構造を名前に反映する"},
    {"noconv", BOOLEAN, TRUE, (void *)&fNoConv, "項目名を大文字に加工しない"},
    {"nofiller", BOOLEAN, TRUE, (void *)&fNoFiller,
     "レコード長調整のFILLERを入れない"},
    {"dir", STRING, TRUE, (void *)&Directory, "ディレクトリファイル"},
    {"record", STRING, TRUE, (void *)&RecordDir, "レコードのあるディレクトリ"},
    {"ddir", STRING, TRUE, (void *)&D_Dir, "定義格納ディレクトリ"},
    {"ld", STRING, TRUE, (void *)&LD_Name, "LD名"},
    {"bd", STRING, TRUE, (void *)&BD_Name, "BD名"},
    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  fNoConv = FALSE;
  fNoFiller = FALSE;
  fFiller = FALSE;
  fSPA = FALSE;
  fLinkage = FALSE;
  fScreen = FALSE;
  fDB = FALSE;
  fDBREC = FALSE;
  fDBCOMM = FALSE;
  fDBPATH = FALSE;
  fMCP = FALSE;
  fFull = FALSE;
  ArraySize = SIZE_DEFAULT_ARRAY_SIZE;
  TextSize = SIZE_DEFAULT_TEXT_SIZE;
  Prefix = "";
  RecName = "";
  LD_Name = NULL;
  BD_Name = NULL;
  fLDW = FALSE;
  fLDR = FALSE;
  fWindowPrefix = FALSE;
  fRecordPrefix = FALSE;
  RecordDir = NULL;
  D_Dir = NULL;
  Directory = "./directory";
  Lang = "OpenCOBOL";
  Top = 1;
}

extern int main(int argc, char **argv) {
  FILE_LIST *fl;
  char *name, *ext;

  SetDefault();
  fl = GetOption(option, argc, argv, NULL);
  InitMessage("copygen", NULL);
  ConvSetLanguage(Lang);
  Conv = NewConvOpt();
  ConvSetSize(Conv, TextSize, ArraySize);
  if (fl != NULL) {
    name = fl->name;
    ext = GetExt(name);
    if (fDBREC) {
      if (strchr(name, '/') != NULL) {
        fprintf(stderr, "error: Invalid db group name: %s\n", name);
        exit(1);
      }
      MakeDBREC(name);
    } else if ((!stricmp(ext, ".rec")) || (!stricmp(ext, ".db"))) {
      MakeFromRecord(name);
    }
  } else {
    if (fScreen) {
      if (LD_Name == NULL) {
        PutLevel(1, TRUE);
        PutString("scrarea");
        printf(".\n");
        PutLevel(2, TRUE);
        PutString("screendata");
        printf(".\n");
      } else {
        MakeLD();
      }
    } else if (fLinkage) {
      MakeLinkage();
    } else if (fDB) {
      MakeDB();
    } else if (fDBCOMM) {
      MakeDBCOMM();
    } else if (fDBPATH) {
      MakeDBPATH();
    } else if (fMCP) {
      MakeMCPAREA();
    } else if (fLDW) {
      fMCP = TRUE;
      fSPA = TRUE;
      fLinkage = TRUE;
      fScreen = TRUE;
      RecName = "ldw";
      Prefix = "ldw-";
      MakeLD();
    } else if (fLDR) {
      fMCP = TRUE;
      fSPA = TRUE;
      fLinkage = TRUE;
      fScreen = TRUE;
      RecName = "ldr";
      Prefix = "ldr-";
      MakeLD();
    } else if (fDBREC) {
      fprintf(stderr, "error: must be set db group name.\n");
      exit(1);
    }
  }
  return (0);
}
