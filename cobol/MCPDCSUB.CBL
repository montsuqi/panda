       IDENTIFICATION          DIVISION.
       PROGRAM-ID.             MCPDCSUB.
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
      *****************************************************************
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
