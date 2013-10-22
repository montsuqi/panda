       IDENTIFICATION          DIVISION.
       PROGRAM-ID.             MCPDCSUB.
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
      *  システム名        : ＯＲＣＡ
      *  サブシステム名    : MCP
      *  コンポーネント名  : DC
      *  管理者            : 
      *  作成日付    作業者        記述
      *  XX.XX.XX    ＮＮＮ        新規作成
      *****************************************************************
       ENVIRONMENT             DIVISION.
      *****************************************************************
       DATA                    DIVISION.
       WORKING-STORAGE             SECTION.
       LINKAGE                 SECTION.
       COPY    MCPAREA.
      *****************************************************************
       PROCEDURE           DIVISION    USING
               MCPAREA.
      *****************************************************************
       000-ENTRY               SECTION.
           EVALUATE    MCP-FUNC
             WHEN     'PUTWINDOW'
               PERFORM 100-PUTWINDOW
             WHEN      OTHER
               DISPLAY    '** FUNCION ERROR'
                   UPON    CONSOLE
           END-EVALUATE.
      *
           EXIT    PROGRAM.
      *****************************************************************
       100-PUTWINDOW           SECTION.
           MOVE   'PUTG'   TO  MCP-STATUS.
           MOVE    ZERO    TO  MCP-RC.
