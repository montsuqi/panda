      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 1993-1999 Ogochan.
      *               2000-2001 Ogochan & JMA (Japan Medical Association).
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
      *   システム名      ：PANDA TPモニタ
      *   サブシステム名  ：COBOL MAIN
      *   管理者          ：ogochan@netlab.jp
      *   日付日付  作業者  記述
      *   00.11.18  ....    修正内容
      *   01.01.08  生越    フォーカス制御追加
      *   01.09.15  生越    JOIN
      ******************************************************************
       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     MCPMAIN.
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
      *    CONSOLE         IS  CONSOLE.
       DATA                DIVISION.
       WORKING-STORAGE         SECTION.
       COPY    MCPAREA.
       COPY    SPAAREA.
       COPY    SCRAREA.
       COPY    LINKAREA.
       COPY    ENUM-VALUE.
      **************************************************************************
       PROCEDURE           DIVISION.
       000-MAIN                SECTION.
           MOVE   'LINK'       TO  MCP-STATUS.
           MOVE    SPACE       TO  MCP-EVENT.
           CALL   'SAMPLE1'    USING
                   MCPAREA
                   SPAAREA
                   LINKAREA
                   SCRAREA.
      *
           STOP    RUN.
