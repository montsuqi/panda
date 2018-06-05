/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include "types.h"
#include "libmondai.h"
#include "styleLex.h"
#include "debug.h"

#define GetChar(fp) fgetc(fp)
#define UnGetChar(fp, c) ungetc((c), (fp))

static struct {
  char *str;
  int token;
} tokentable[] = {{"style", T_STYLE},
                  {"fontset", T_FONTSET},
                  {"fg", T_FG},
                  {"bg", T_BG},
                  {"bg_pixmap", T_BG_PIXMAP},
                  {"text", T_TEXT},
                  {"base", T_BASE},
                  {"light", T_LIGHT},
                  {"dark", T_DARK},
                  {"mid", T_MID},
                  {"normal", T_NORMAL},
                  {"NORMAL", T_NORMAL},
                  {"prelight", T_PRELIGHT},
                  {"PRELIGHT", T_PRELIGHT},
                  {"insensitive", T_INSENSITIVE},
                  {"INSENSITIVE", T_INSENSITIVE},
                  {"selected", T_SELECTED},
                  {"SELECTED", T_SELECTED},
                  {"active", T_ACTIVE},
                  {"ACTIVE", T_ACTIVE},
                  {"", 0}};

static GHashTable *Reserved = NULL;

extern void StyleLexInit(void) {
  int i;

  if (Reserved == NULL) {
    Reserved = NewNameHash();
    for (i = 0; tokentable[i].token != 0; i++) {
      g_hash_table_insert(Reserved, tokentable[i].str,
                          (gpointer)(long)tokentable[i].token);
    }
  }
}

static int CheckReserved(char *str) {
  gpointer p;
  int ret;

  if ((p = g_hash_table_lookup(Reserved, str)) != NULL) {
    ret = (int)(long)p;
  } else {
    ret = T_SYMBOL;
  }
  return (ret);
}

extern int StyleLex(Bool fSymbol) {
  int c, len;
  int token;
  char *s;
  Bool fFloat;

  dbgmsg(">StyleLex");
retry:
  while (isspace(c = GetChar(StyleFile))) {
    if (c == '\n') {
      c = ' ';
      StylecLine++;
    }
  }
  if (c == '#') {
    while ((c = GetChar(StyleFile)) != '\n')
      ;
    StylecLine++;
    goto retry;
  }
  if (c == '"') {
    s = StyleComSymbol;
    len = 0;
    while ((c = GetChar(StyleFile)) != '"') {
      if (c == '\\') {
        c = GetChar(StyleFile);
      }
      *s = c;
      if (len < SIZE_SYMBOL) {
        s++;
        len++;
      }
    }
    *s = 0;
    token = T_SCONST;
  } else if (isalpha(c)) {
    s = StyleComSymbol;
    len = 0;
    do {
      *s = c;
      if (len < SIZE_SYMBOL) {
        s++;
        len++;
      }
      c = GetChar(StyleFile);
    } while ((isalpha(c)) || (isdigit(c)) || (c == '.') || (c == '_'));
    *s = 0;
    UnGetChar(StyleFile, c);
    if (fSymbol) {
      token = T_SYMBOL;
    } else {
      token = CheckReserved(StyleComSymbol);
    }
  } else if (isdigit(c)) {
    s = StyleComSymbol;
    len = 0;
    fFloat = FALSE;
    do {
      *s = c;
      if (c == '.') {
        if (fFloat) {
          printf(". duplicate\n");
        }
        fFloat = TRUE;
      }
      if (len < SIZE_SYMBOL) {
        s++;
        len++;
      }
      c = GetChar(StyleFile);
    } while ((isdigit(c)) || (c == '.'));
    *s = 0;
    UnGetChar(StyleFile, c);
    if (fFloat) {
      StyleComInt = (int)(atof(StyleComSymbol) * 65535.0);
    } else {
      StyleComInt = atoi(StyleComSymbol);
    }
    token = T_ICONST;
  } else {
    switch (c) {
    case EOF:
      token = T_EOF;
      break;
    default:
      token = c;
      break;
    }
  }
  dbgmsg("<StyleLex");
  return (token);
}
