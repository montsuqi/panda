       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     TPDEMO2.
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
      *   コンポーネント名：デモ2
      *   管理者          ：ogochan@NetLab.jp
      *   日付日付  作業者  記述
      *   00.11.17  ....    修正内容
      ******************************************************************
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
           CONSOLE         IS  CONSOLE.
       DATA                DIVISION.
       WORKING-STORAGE         SECTION.
       01  FLG.
           02  FLG-EOF     PIC 9.
       01  WRK.
           02  WRK-BUFF    PIC X(30).
       01  I           PIC S9(9)   BINARY.
       COPY    ENUM-VALUE.
       COPY    DB-META.
       LINKAGE                 SECTION.
       COPY    MCPAREA.
       COPY    SPAAREA.
       COPY    SCRAREA.
       COPY    LINKAREA.
      **************************************************************************
       PROCEDURE           DIVISION    USING
           MCPAREA
           SPAAREA
           LINKAREA
           SCRAREA.
       000-MAIN                SECTION.
           DISPLAY    '*** TPDEMO2'
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
      **************************************************************************
       010-INIT                SECTION.
           IF          (  LNK-LINKTEXT  =  SPACE  )
                   OR  (  LNK-LINKTEXT  =  LOW-VALUE  )
               MOVE   '漢字を入れてみた'   TO  WN3-TEXT1-VALUE
             ELSE
               MOVE    LNK-LINKTEXT        TO  WN3-TEXT1-VALUE
           END-IF.
      *
           MOVE   'blue'       TO  WN3-TEXT1-STYLE.
      *
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'project3'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       210-QUIT                SECTION.
           DISPLAY   '[' WN3-TEXT1 ']'
               UPON    CONSOLE.
           MOVE   WN3-TEXT1-VALUE  TO  LNK-LINKTEXT.
      *
           MOVE   'CHANGE'     TO  MCP-PUTTYPE.
           MOVE   'project1'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       290-OTHER               SECTION.
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'project3'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       900-PUT-WINDOW          SECTION.
           MOVE   'PUTWINDOW'  TO  MCP-FUNC.
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
