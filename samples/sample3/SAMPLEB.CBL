       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     SAMPLEB.
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
      *   コンポーネント名：バッチサンプル
      *   管理者          ：ogochan@NetLab.jp
      *   日付日付  作業者  記述
      *   01.05.28  ....    修正内容
      ******************************************************************
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
           CONSOLE         IS  CONSOLE.
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
       01  I           PIC S9(9)   BINARY.
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
       COPY    DBPATH.
       COPY    DB-META.
       COPY    DB-ADRS.
       COPY    MCPAREA.
       LINKAGE                 SECTION.
       01  PARA.
           02  FILLER      PIC X(256).
      **************************************************************************
       PROCEDURE           DIVISION
               USING
           PARA.
       000-MAIN                SECTION.
           DISPLAY    '*** SAMPLEB'
               UPON    CONSOLE.
      *
           PERFORM 010-INIT.
           PERFORM 100-DUMP.
           PERFORM 200-INSERT.
           PERFORM 090-FINISH.
      *
           STOP    RUN.
      **************************************************************************
       010-INIT                SECTION.
           OPEN    OUTPUT
               LOG-FILE.
           PERFORM 800-DBOPEN.
           PERFORM 800-DBSTART.
      **************************************************************************
       090-FINISH              SECTION.
           PERFORM 800-DBCOMMIT.
           PERFORM 800-DBDISCONNECT.
      *
           CLOSE
               LOG-FILE.
      **************************************************************************
       100-DUMP                SECTION.
           MOVE   'ogochan%'   TO  ADR-HOME.
           PERFORM 800-DBSELECT-ADRS.
      *
           PERFORM 800-DBFETCH-ADRS.
           PERFORM
                   UNTIL   MCP-RC  NOT =  ZERO
               MOVE    ADR-NAME    TO  EDT-NAME
               MOVE    ADR-TEL     TO  EDT-TEL
               MOVE    ADR-TOLL    TO  EDT-TOLL
               MOVE    EDT-1       TO  LOG-DATA
               MOVE   X'0d'        TO  LOG-CR
               WRITE   LOG-REC
               MOVE    ADR-ADDRESS(1)  TO  EDT-ADDRESS1
               MOVE    ADR-ADDRESS(2)  TO  EDT-ADDRESS2
               MOVE    ADR-ADDRESS(3)  TO  EDT-ADDRESS3
               MOVE    EDT-2       TO  LOG-DATA
               MOVE   X'0d'        TO  LOG-CR
               WRITE   LOG-REC
               IF      ADR-WEIGHT  =  72.3
                   MOVE    72.5        TO  ADR-WEIGHT
                   MOVE   '090-8101-7514'  TO  ADR-TEL
                   MOVE   '円山町'     TO  ADR-ADDRESS(2)
                   MOVE   '2-16'       TO  ADR-ADDRESS(3)
                   PERFORM 800-DBUPDATE-ADRS
               END-IF
               IF      ADR-WEIGHT  =  72.3
                   PERFORM 800-DBDELETE-ADRS
               END-IF
               PERFORM 800-DBFETCH-ADRS
           END-PERFORM.
      **************************************************************************
       200-INSERT              SECTION.
           MOVE   'ogochanK'   TO  ADR-NAME.
           MOVE   '070-6163-7932'  TO  ADR-TEL.
           MOVE   'ogochan'    TO  ADR-HOME.
           MOVE    165.0       TO  ADR-TOLL.
           MOVE    71.3        TO  ADR-WEIGHT.
           MOVE   '渋谷区'     TO  ADR-ADDRESS(1).
           MOVE   '神宮前'     TO  ADR-ADDRESS(2).
           MOVE   '1-20-4'     TO  ADR-ADDRESS(3).
      *
           PERFORM 800-DBINSERT-ADRS.
      **************************************************************************
       800-DBOPEN              SECTION.
           MOVE   'DBOPEN'     TO  MCP-FUNC.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
      **************************************************************************
       800-DBSTART             SECTION.
           MOVE   'DBSTART'    TO  MCP-FUNC.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
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
      *
      **************************************************************************
       800-DBUPDATE-ADRS       SECTION.
      *    MOVE   'DBUPDATE'   TO  MCP-FUNC.
           MOVE   'DBDELETEINSERT' TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      **************************************************************************
       800-DBDELETE-ADRS       SECTION.
           MOVE   'DBDELETE'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      **************************************************************************
       800-DBINSERT-ADRS       SECTION.
           MOVE   'DBINSERT'   TO  MCP-FUNC.
           MOVE    PATH-ADRS-MAIL  TO  MCP-PATH.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                ADRS.
      **************************************************************************
       800-DBCOMMIT            SECTION.
           MOVE   'DBCOMMIT'   TO  MCP-FUNC.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
      **************************************************************************
       800-DBDISCONNECT        SECTION.
           MOVE   'DBDISCONNECT'    TO  MCP-FUNC.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
