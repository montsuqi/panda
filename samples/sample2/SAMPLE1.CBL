       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     SAMPLE1.
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
      *   コンポーネント名：サンプル
      *   管理者          ：ogochan@NetLab.jp
      *   日付日付  作業者  記述
      *   01.02.11  ....    修正内容
      ******************************************************************
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
      *    CONSOLE         IS  CONSOLE.
       DATA                DIVISION.
       WORKING-STORAGE         SECTION.
       01  FLG.
           02  FLG-EOF     PIC 9.
       01  WRK.
           02  WRK-BUFF    PIC X(30).
       01  I           PIC S9(9)   BINARY.
       COPY    ENUM-VALUE.
       COPY    DBPATH.
       COPY    DB-ADRS.
       COPY    DB-META.
       LINKAGE                 SECTION.
       COPY    MCPAREA.
       COPY    SPAAREA.
       COPY    SCRAREA.
       COPY    LINKAREA.
      **************************************************************************
       PROCEDURE           DIVISION   USING
           MCPAREA
           SPAAREA
           LINKAREA
           SCRAREA.
       000-MAIN                SECTION.
           DISPLAY    '*** SAMPLE1 ' MCP-STATUS MCP-EVENT
               UPON    CONSOLE.
      *
           EVALUATE    MCP-STATUS      ALSO    MCP-EVENT
             WHEN     'LINK'           ALSO    ANY
               PERFORM 010-INIT
             WHEN     'PUTG'           ALSO   'Search'
               PERFORM 200-SEARCH
             WHEN     'PUTG'           ALSO   'Quit'
               PERFORM 270-QUIT
             WHEN      OTHER
               PERFORM 290-OTHER
           END-EVALUATE.
      *
           DISPLAY   'MCP-WINDOW:[' MCP-WINDOW ']'
               UPON    CONSOLE.
      *
           EXIT    PROGRAM.
      **************************************************************************
       010-INIT                SECTION.
           MOVE    SPACE       TO  LIST-KEY.
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (   I   >  20  )
               MOVE    SPACE       TO  LIST-NAME(I)
                                       LIST-TEL(I)
                                       LIST-MAIL(I)
                                       LIST-ADRS(I)
               MOVE   'F'          TO  LIST-SELECT(I)
           END-PERFORM.
      *
           PERFORM 800-GET-ADRS.
           MOVE    META-HOST  TO  LIST-KEY.
           DISPLAY   'META-HOST:[' META-HOST ']'
               UPON    CONSOLE.
      *
      *    MOVE   'NEW'       TO  MCP-PUTTYPE.
           MOVE    SPACE      TO  MCP-PUTTYPE.
           MOVE   'list'      TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       200-SEARCH              SECTION.
           DISPLAY   '**  LIST-KEY = [' LIST-KEY ']'
               UPON    CONSOLE.
      *
           MOVE    LIST-KEY    TO  ADR-HOME.
           PERFORM 800-DBSELECT-ADRS.
      *
           PERFORM 800-DBFETCH-ADRS.
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (  MCP-RC  NOT =  ZERO  )
                       OR  (  I       >      20    )
               DISPLAY   '**  ADR-NAME = [' ADR-NAME ']'
                   UPON    CONSOLE
               MOVE    ADR-NAME    TO  LIST-NAME(I)
               MOVE    ADR-TEL     TO  LIST-TEL(I)
               MOVE    ADR-HOME    TO  LIST-MAIL(I)
               MOVE    ADR-ADDRESS(1)  TO  LIST-ADRS(I)
               MOVE   'F'              TO  LIST-SELECT(I)
               PERFORM 800-DBFETCH-ADRS
           END-PERFORM.
           COMPUTE LIST-COUNT = I - 1.
      *
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'list'      TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       270-QUIT                SECTION.
           MOVE   'CLOSE'      TO  MCP-PUTTYPE.
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       290-OTHER               SECTION.
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'list'       TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       800-DBSELECT-ADRS       SECTION.
           MOVE   'DBSELECT'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL   TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      **************************************************************************
       800-DBFETCH-ADRS        SECTION.
           MOVE   'DBFETCH'    TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
           INITIALIZE  ADRS.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      **************************************************************************
       800-GET-ADRS            SECTION.
           MOVE   'DBFETCH'    TO  MCP-FUNC.
           MOVE    PATH-METADB-PRIMARY TO  MCP-PATH.
           INITIALIZE  METADB.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
      **************************************************************************
       900-PUT-WINDOW          SECTION.
           MOVE   'PUTWINDOW'  TO  MCP-FUNC.
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
