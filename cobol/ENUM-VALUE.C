#include	"enum.h"
#include	"glterm.h"
start-cobol
      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 1993-1999 Ogochan.
      *               2000-2002 Ogochan & JMARI.
      *
      * This module is part of PANDA.
      *
      *     PANDA is distributed in the hope that it will be useful, but
      * WITHOUT ANY WARRANTY.  No author or distributor accepts 
      * responsibility to anyone for the consequences of using it or for
      * whether it serves any particular purpose or works at all, unless
      * he says so in writing.
      * Refer to the GNU General Public License for full details. 
      *
      *     Everyone is granted permission to copy, modify and
      * redistribute PANDA, but only under the conditions described in
      * the GNU General Public License.  A copy of this license is
      * supposed to have been given to you along with PANDA so you can
      * know your rights and responsibilities.  It should be in a file
      * named COPYING.  Among other things, the copyright notice and 
      * this notice must be preserved on all copies. 
      ******************************************************************
      *    ENUM-VALUES
      ******************************************************************
       01  ENUM-VALUES.
           02  WIDGET-STATE.
             03  WIDGET-NORMAL   PIC S9(9)   BINARY  VALUE   WIDGET_STATE_NORMAL.
             03  WIDGET-ACTIVE   PIC S9(9)   BINARY  VALUE   WIDGET_STATE_ACTIVE.
             03  WIDGET-PRELIGHT PIC S9(9)   BINARY  VALUE   WIDGET_STATE_PRELIGHT.
             03  WIDGET-SELECTED PIC S9(9)   BINARY  VALUE   WIDGET_STATE_SELECTED.
             03  WIDGET-INSENSITIVE  PIC S9(9)   BINARY  VALUE   WIDGET_STATE_INSENSITIVE.
           02  MCP-CODE.
             03  MCP-OK          PIC S9(9)   BINARY  VALUE   MCP_OK.
             03  MCP-EOF         PIC S9(9)   BINARY  VALUE   MCP_EOF.
             03  MCP-NONFATAL    PIC S9(9)   BINARY  VALUE   MCP_NONFATAL.
             03  MCP-BAD-ARG     PIC S9(9)   BINARY  VALUE   MCP_BAD_ARG.
             03  MCP-BAD-FUNC    PIC S9(9)   BINARY  VALUE   MCP_BAD_FUNC.
             03  MCP-BAD-SQL     PIC S9(9)   BINARY  VALUE   MCP_BAD_SQL.
             03  MCP-BAD-OTHER   PIC S9(9)   BINARY  VALUE   MCP_BAD_OTHER.
           02  MCP-CTL.
             03  MCP-CTL-NONE       PIC S9(9)   BINARY  VALUE   SCREEN_NULL.
             03  MCP-CTL-CURRENT    PIC S9(9)   BINARY  VALUE   SCREEN_CURRENT_WINDOW.
             03  MCP-CTL-NEW        PIC S9(9)   BINARY  VALUE   SCREEN_NEW_WINDOW.
             03  MCP-CTL-CLOSE      PIC S9(9)   BINARY  VALUE   SCREEN_CLOSE_WINDOW.
             03  MCP-CTL-CHANGE     PIC S9(9)   BINARY  VALUE   SCREEN_CHANGE_WINDOW.
             03  MCP-CTL-JOIN       PIC S9(9)   BINARY  VALUE   SCREEN_JOIN_WINDOW.
             03  MCP-CTL-END        PIC S9(9)   BINARY  VALUE   SCREEN_END_SESSION.
