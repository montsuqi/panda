       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     TPDEMO7.
      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 2000-2001 Ogochan & JMA (Japan Medical Association).
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
      *   コンポーネント名：耐久試験用プログラム (COBOL)
      *   管理者          ：ogochan@NetLab.jp
      *   日付日付  作業者  記述
      *   01.12.18  紀國    新規
      *   01.12.25  紀國    イベント処理修正
      *   02.01.09  紀國    DBイベント追加
      *   02.01.11  紀國    Open COBOL へ移行	
      *   02.01.28  紀國    バッチ処理追加
      ******************************************************************
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
      *     CONSOLE         IS  CONSOLE.
       INPUT-OUTPUT            SECTION.
       FILE-CONTROL.
           SELECT  LOG-FILE
               ASSIGN  TO  "log"
               ORGANIZATION    SEQUENTIAL
               ACCESS  MODE    SEQUENTIAL.
       DATA                DIVISION.
       FILE                    SECTION.
       FD  LOG-FILE.
       01  LOG-REC.
           02  LOG-DATA    PIC X(127).
           02  LOG-CR      PIC X.
       WORKING-STORAGE         SECTION.
       01  FLG.
           02  FLG-EOF     PIC 9.
       01  WRK.
           02  WRK-BUFF    PIC X(30).
       01  WRK-CAL-YMD.
           03  WRK-CAL-YEAR        PIC 9(04).
           03  WRK-CAL-MONTH       PIC 9(02).
           03  WRK-CAL-DAY         PIC 9(02).
       01  WRK-NUMBER1     PIC X(10).
       01  WRK-PANDACOMBO          PIC X(20).
      *
       01  EDT.
           02  EDT-1.
             03  EDT-NAME    PIC X(20).
             03  FILLER          PIC X       VALUE  ':'.
             03  EDT-TEL     PIC X(13).
             03  FILLER          PIC X       VALUE  ':'.
             03  EDT-TOLL    PIC ZZZ9.9.
           02  EDT-2.
             03  EDT-ADDRESS1    PIC X(20).
             03  FILLER          PIC X       VALUE  ':'.
             03  EDT-ADDRESS2    PIC X(20).
             03  FILLER          PIC X       VALUE  ':'.
             03  EDT-ADDRESS3    PIC X(20).
      *
       01  I           PIC S9(9)   BINARY.
       COPY    ENUM-VALUE.
       COPY    DBPATH.
       COPY    DB-ADRS.
       COPY    DB-META.
       COPY    DB-SHELL.
       LINKAGE                 SECTION.
       COPY    MCPAREA.
       COPY    SPAAREA.
       COPY    SCRAREA.
       COPY    LINKAREA.
      ******************************************************************
       PROCEDURE           DIVISION   USING
           MCPAREA
           SPAAREA
           LINKAREA
           SCRAREA.
       000-MAIN                SECTION.
           DISPLAY    '*** PROJECT6'
               UPON    CONSOLE.
      *
           IF      MCP-STATUS  =  'PUTG'
               DISPLAY    '*** PAGE = '    PAGENO
                   UPON    CONSOLE
           END-IF.
           EVALUATE    MCP-STATUS      ALSO    MCP-EVENT
             WHEN     'LINK'           ALSO    ANY
               PERFORM 010-INIT
             WHEN     'PUTG'           ALSO   'reset'
               PERFORM 210-RESET
             WHEN     'PUTG'           ALSO   'reset2'
               PERFORM 220-RESET2
             WHEN     'PUTG'           ALSO   'insert'
               PERFORM 230-INSERT
             WHEN     'PUTG'           ALSO   'insert2'
               PERFORM 240-INSERT2
             WHEN     'PUTG'           ALSO   'SELECT'
               PERFORM 250-SELECT
             WHEN     'PUTG'           ALSO   'SELECT2'
               PERFORM 260-SELECT2
             WHEN     'PUTG'           ALSO   'CHANGED'
               PERFORM 270-CHANGED
             WHEN     'PUTG'           ALSO   'CLICKED'
               PERFORM 280-CLICKED
             WHEN     'PUTG'           ALSO   'Search'
               PERFORM 281-SEARCH
             WHEN     'PUTG'           ALSO   'Insert'
               PERFORM 282-INSERT
             WHEN     'PUTG'           ALSO   'Delete'
               PERFORM 283-DELETE
             WHEN     'PUTG'           ALSO   'update'
               PERFORM 284-UPDATE
             WHEN     'PUTG'           ALSO   'LD'
               PERFORM 285-CALOPEN
             WHEN     'PUTG'           ALSO   'Shell'
               PERFORM 286-SHELL
             WHEN     'PUTG'           ALSO   'Batch_Insert'
               PERFORM 287-BATCH-INSERT
             WHEN     'PUTG'           ALSO   'Batch_Delete'
               PERFORM 288-BATCH-DELETE
             WHEN     'PUTG'           ALSO   'Batch_Update'
               PERFORM 289-BATCH-UPDATE
             WHEN     'PUTG'           ALSO   'ACTIVATE'
               PERFORM 290-ACTIVATE
             WHEN      OTHER
               PERFORM 299-OTHER
            END-EVALUATE.
      *
           EXIT    PROGRAM.
      ******************************************************************
       010-INIT                SECTION.
      *     MOVE   'kida'       TO  ENTRY7-VALUE.
           MOVE   'testtest'   TO  TEXT1-VALUE.
           MOVE   'T'          TO  RADIOBUTTON2-VALUE.
           MOVE   'test1'      TO  COMBO1-ITEM(1).
           MOVE   'test2'      TO  COMBO1-ITEM(2).
           MOVE   'test3'      TO  COMBO1-ITEM(3).
           MOVE   'test4'      TO  COMBO1-ITEM(4).
           MOVE    4           TO  COMBO1-COUNT.
      *
           MOVE   '2001'       TO  CALENDAR1-YEAR.
           MOVE   '11'         TO  CALENDAR1-MONTH.
           MOVE   '11'         TO  CALENDAR1-DAY.
      *
           MOVE   'test1'      TO  PANDACOMBO1-ITEM(1).
           MOVE   'test2'      TO  PANDACOMBO1-ITEM(2).
           MOVE    4           TO  PANDACOMBO1-COUNT.
      *
           MOVE    '%'    TO  ADR-HOME.
           PERFORM 800-DBSELECT-ADRS.
           PERFORM 800-DBFETCH-ADRS.
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (  MCP-RC  NOT =  ZERO  )
                       OR  (  I       >      20    )
                 DISPLAY   '**  ADR-NAME = [' ADR-NAME ']'
                     UPON    CONSOLE
                 MOVE    ADR-NAME    TO  PANDACOMBO2-ITEM(I)
                 PERFORM 800-DBFETCH-ADRS
           END-PERFORM.
           COMPUTE PANDACOMBO2-COUNT = I - 1.
      *
           MOVE    SPACE      TO  MCP-PUTTYPE.
           MOVE   'project6'   TO  MCP-WINDOW.              
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       210-RESET               SECTION.
           MOVE   ' '          TO  ENTRY7-VALUE.
           MOVE   'F'          TO  CHECKBUTTON2-VALUE.
           MOVE   'F'          TO  CHECKBUTTON3-VALUE.
           MOVE   'T'          TO  RADIOBUTTON2-VALUE.
           MOVE   'F'          TO  RADIOBUTTON3-VALUE.
           MOVE   'testtest'   TO  TEXT1-VALUE.             
      *
           MOVE   'project6'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       220-RESET2              SECTION.
           MOVE   ' '          TO  PANDAENTRY1-VALUE.
           MOVE   0            TO  NUMBERENTRY2-VALUE.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       230-INSERT              SECTION.  
           IF      TOGGLEBUTTON2-VALUE  =  'T'
               COMPUTE SPA-COUNT = SPA-COUNT + 1
               MOVE    TEXT1-VALUE  TO  CLIST3-VALUE1(SPA-COUNT)
               IF      RADIOBUTTON2-VALUE  =  'T'
                   MOVE   'panda'       TO  CLIST3-VALUE2(SPA-COUNT)
                 ELSE
                   MOVE   'zebra'       TO  CLIST3-VALUE2(SPA-COUNT)
               END-IF
               DISPLAY    '*** TOGGLED1 = ' SPA-COUNT              
             ELSE
               COMPUTE SPA-COUNT = SPA-COUNT + 1
               MOVE    TEXT1-VALUE  TO  CLIST3-VALUE1(SPA-COUNT)
               IF      CHECKBUTTON2-VALUE  =  'T'
                   MOVE   'check1'      TO  CLIST3-VALUE2(SPA-COUNT)
                 ELSE
                   MOVE   'check2'      TO  CLIST3-VALUE2(SPA-COUNT)
               END-IF
               DISPLAY    '*** TOGGLED2 = ' SPA-COUNT              
           END-IF.
      *
           MOVE    ENTRY7-VALUE     TO  CLIST3-VALUE3(SPA-COUNT).
           MOVE    SPA-COUNT   TO  CLIST3-COUNT.
           MOVE   'project6'   TO  MCP-WINDOW.  
           MOVE    SPACE       TO  MCP-PUTTYPE.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       240-INSERT2             SECTION.
           COMPUTE SPA-COUNT = SPA-COUNT + 1
           MOVE   PANDAENTRY1-VALUE    TO  PANDACLIST1-VALUE1(SPA-COUNT).
           MOVE   NUMBERENTRY2-VALUE-X TO  PANDACLIST1-VALUE2(SPA-COUNT).
           MOVE   PANDACOMBO1-VALUE    TO  WRK-PANDACOMBO.
           MOVE   WRK-PANDACOMBO       TO  PANDACLIST1-VALUE3(SPA-COUNT).
           MOVE    SPA-COUNT   TO  PANDACLIST1-COUNT.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       250-SELECT              SECTION.  
           MOVE   ' '           TO  PANDAENTRY1-VALUE.
           MOVE   ' '           TO  NUMBERENTRY2-VALUE.  
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       260-SELECT2             SECTION.  
           MOVE   ' '           TO  TEXT1-VALUE.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       270-CHANGED             SECTION.  
           MOVE   ' '           TO  TEXT1-VALUE.
           MOVE   'project6'   TO  MCP-WINDOW.  
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       280-CLICKED             SECTION.
           MOVE    CALENDAR1-YEAR   TO  WRK-CAL-YEAR.
           MOVE    CALENDAR1-MONTH  TO  WRK-CAL-MONTH.
           MOVE    CALENDAR1-DAY    TO  WRK-CAL-DAY.
      *
           MOVE    WRK-CAL-YEAR         TO  ENTRY4-VALUE.
           MOVE    WRK-CAL-MONTH        TO  ENTRY5-VALUE.
           MOVE    WRK-CAL-DAY          TO  ENTRY6-VALUE.
      *
           MOVE   'project6'   TO  MCP-WINDOW.  
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       281-SEARCH            SECTION.  
           MOVE    ENTRY8    TO  ADR-HOME.
           PERFORM 800-DBSELECT-ADRS.
      *
           PERFORM 800-DBFETCH-ADRS.
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (  MCP-RC  NOT =  ZERO  )
                       OR  (  I       >      20    )
                 DISPLAY   '**  ADR-NAME = [' ADR-NAME ']'
                     UPON    CONSOLE
                 MOVE    ADR-NAME    TO  CLIST4-VALUE1(I)
                 MOVE    ADR-TEL     TO  CLIST4-VALUE2(I)
                 MOVE    ADR-HOME    TO  CLIST4-VALUE3(I)
                 MOVE   'F'          TO  CLIST4-SELECT(I)
                 PERFORM 800-DBFETCH-ADRS
           END-PERFORM.
           COMPUTE CLIST4-COUNT = I - 1.
      *
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'project6'      TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       282-INSERT            SECTION.  
           OPEN    OUTPUT
               LOG-FILE.
      *
           MOVE   ENTRY9      TO  ADR-NAME.
           MOVE   ENTRY10     TO  ADR-TEL.
           MOVE   ENTRY11     TO  ADR-HOME.
           MOVE    175.0      TO  ADR-TOLL.
           MOVE    63.0       TO  ADR-WEIGHT.
           MOVE   '松江市'    TO  ADR-ADDRESS(1).
           MOVE   '学園南'    TO  ADR-ADDRESS(2).
           MOVE   '2丁目'     TO  ADR-ADDRESS(3).
      *
           DISPLAY    '*** ADR-NAME = '    ADR-NAME.
           DISPLAY    '*** ADR-TEL = '     ADR-TEL.
           DISPLAY    '*** ADR-HOME = '    ADR-HOME.
      *
           MOVE   'DBINSERT'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      *
           MOVE   'DBCOMMIT'   TO  MCP-FUNC.
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
      *
           CLOSE
               LOG-FILE.
      *
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'project6'   TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      *     EXIT.
      ******************************************************************
       283-DELETE            SECTION.  
           DISPLAY    '*** PANDACOMBO-VALUE = '    PANDACOMBO2-VALUE.
           MOVE   PANDACOMBO2-VALUE  TO  WRK-PANDACOMBO.
           DISPLAY    '*** WRK-PANDACOMBO = '    WRK-PANDACOMBO.
           MOVE   WRK-PANDACOMBO     TO  ADR-NAME.
      *
           MOVE   'DBDELETE'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
           MOVE   'project6'   TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       284-UPDATE            SECTION.  
           MOVE   ENTRY12   TO  ADR-HOME.
      *
           PERFORM 800-DBSELECT-ADRS.
           PERFORM 800-DBFETCH-ADRS.
           PERFORM UNTIL   MCP-RC  NOT =  ZERO
               MOVE    ENTRY13    TO    ADR-NAME
               PERFORM 800-DBUPDATE-ADRS
               PERFORM 800-DBFETCH-ADRS
           END-PERFORM.
      *
           MOVE   'project6'   TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       285-CALOPEN                 SECTION.
           MOVE   "CHANGE"     TO  MCP-PUTTYPE.
           MOVE   'project5'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       286-SHELL               SECTION.
           DISPLAY   '**  CLICK SHELL KICK'
               UPON    CONSOLE.
      *
           MOVE    ENTRY14    TO  SHELL-ARG.
           MOVE   'CAT'       TO  MCP-FUNC.
           MOVE    PATH-SHELL-DO   TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                SHELL.
      *
           MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE   'project6'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       287-BATCH-INSERT        SECTION.
           MOVE '~/panda/samples/sample4/batch1.sh' TO SHELL-ARG.
           MOVE   'SH'       TO  MCP-FUNC.
           MOVE    PATH-SHELL-DO   TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                SHELL.
      *
           MOVE   'project6'   TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       288-BATCH-DELETE        SECTION.
           MOVE '~/panda/samples/sample4/batch2.sh' TO SHELL-ARG.
           MOVE   'SH'       TO  MCP-FUNC.
           MOVE    PATH-SHELL-DO   TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                SHELL.
      *
           MOVE   'project6'   TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       289-BATCH-UPDATE        SECTION.
           MOVE '~/panda/samples/sample4/batch3.sh' TO SHELL-ARG.
           MOVE   'SH'       TO  MCP-FUNC.
           MOVE    PATH-SHELL-DO   TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                SHELL.
      *
           MOVE   'project6'   TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       290-ACTIVATE            SECTION.  
           COMPUTE SPA-COUNT = SPA-COUNT + 1
           MOVE    PANDAENTRY1-VALUE
               TO  PANDACLIST1-VALUE1(SPA-COUNT).
           MOVE   NUMBERENTRY2-VALUE-X 
               TO  PANDACLIST1-VALUE2(SPA-COUNT).
           MOVE    PANDACOMBO1-VALUE
               TO  PANDACLIST1-VALUE3(SPA-COUNT).
           MOVE    SPA-COUNT   TO  PANDACLIST1-COUNT.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       299-OTHER               SECTION.
      *    MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'project6'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      ******************************************************************
       800-DBSELECT-ADRS       SECTION.
           MOVE   'DBSELECT'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL   TO  MCP-PATH.
      *     MOVE    10   TO  MCP-PNAME.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      ******************************************************************
       800-DBFETCH-ADRS        SECTION.
           MOVE   'DBFETCH'    TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
           INITIALIZE  ADRS.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS. 
      ******************************************************************
       800-DBUPDATE-ADRS       SECTION.
      *    MOVE   'DBUPDATE'   TO  MCP-FUNC.
           MOVE   'DBDELETEINSERT' TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      ******************************************************************
       800-DBDELETE-ADRS       SECTION.
           MOVE   'DBDELETE'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      ******************************************************************
       900-PUT-WINDOW          SECTION.
           MOVE   'PUTWINDOW'  TO  MCP-FUNC.
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.



