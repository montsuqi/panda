       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     TPDEMO3.
      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).
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
      *   サブシステム名  ：デモ
      *   コンポーネント名：デモ3
      *   管理者          ：
      *   日付日付  作業者  記述
      *   01.01.04  ....    修正内容
      *   02.01.18  紀國    処理一部変更
      ******************************************************************
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
      *     CONSOLE         IS  CONSOLE.
       DATA                DIVISION.
       WORKING-STORAGE         SECTION.
       01  EDT.
           02  YMD.
             03  YMD-YEAR    PIC 9(4).
             03  FILLER      PIC X       VALUE  '/'.
             03  YMD-MONTH   PIC 9(2).
             03  FILLER      PIC X       VALUE  '/'.
             03  YMD-DAY     PIC 9(2).
       COPY    ENUM-VALUE.
       COPY    DB-META.
       LINKAGE                 SECTION.
       COPY    MCPAREA.
       COPY    SPAAREA.
       COPY    SCRAREA.
       COPY    LINKAREA.
      ******************************************************************
       PROCEDURE           DIVISION    USING
           MCPAREA
           SPAAREA
           LINKAREA
           SCRAREA.
       000-MAIN                SECTION.
           DISPLAY    '*** TPDEMO3'
               UPON    CONSOLE.
      *
           EVALUATE    MCP-STATUS      ALSO    MCP-EVENT
             WHEN     'LINK'           ALSO    ANY
               PERFORM 010-INIT
             WHEN     'PUTG'           ALSO   'Quit'
               PERFORM 210-QUIT
             WHEN      OTHER
               PERFORM 290-OTHER
           END-EVALUATE.
      *
           EXIT    PROGRAM.
      ******************************************************************
       010-INIT                SECTION.
           MOVE   2002         TO  WN5-YEAR.
           MOVE   1            TO  WN5-MONTH.
           MOVE   28           TO  WN5-DAY.
      *
           MOVE   "CURRENT"    TO  MCP-PUTTYPE.
           MOVE   "project5"   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
       010-INIT-EXT.
           EXIT.
      ******************************************************************
       210-QUIT                SECTION.
           MOVE   "CHANGE"     TO  MCP-PUTTYPE.
           MOVE   'project6'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      *
       210-QUIT-EXT.
           EXIT.
      ******************************************************************
       290-OTHER               SECTION.
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'project5'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
       290-OTHER-EXT.
           EXIT.
      ******************************************************************
       900-PUT-WINDOW          SECTION.
           MOVE   'PUTWINDOW'  TO  MCP-FUNC.
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
      *
           .
       900-PUT-WINDOW-EXT.
           EXIT.


