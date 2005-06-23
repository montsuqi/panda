#include	"enum.h"
#include	"glterm.h"
start-cobol
      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 1993-1999 Ogochan.
      *               2000-2003 Ogochan & JMARI.
      *               2004-2005 Ogochan.
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
