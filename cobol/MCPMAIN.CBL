      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 1993-1999 Ogochan.
      *               2000-2002 Ogochan & JMA.
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
       INPUT-OUTPUT            SECTION.
       FILE-CONTROL.
           SELECT  LDR-FILE
               ASSIGN  TO  "dc.input"
               ORGANIZATION    SEQUENTIAL
               ACCESS  MODE    SEQUENTIAL.
           SELECT  LDW-FILE
               ASSIGN  TO  "dc.output"
               ORGANIZATION    SEQUENTIAL
               ACCESS  MODE    SEQUENTIAL.
       DATA                DIVISION.
       FILE                    SECTION.
       FD  LDR-FILE.
       01  LDR-REC.
           02  FILLER      PIC X(1024).
       FD  LDW-FILE.
       01  LDW-REC.
           02  FILLER      PIC X(1024).
       WORKING-STORAGE         SECTION.
       COPY    DB-META.
       COPY    LDRFILE.
       COPY    LDWFILE.
       COPY    ENUM-VALUE.
       01  WORK.
           02  WRK-PROG    PIC X(16).
           02  WRK-WINEND  PIC S9(9)   BINARY.
           02  WRK-WINFR   PIC S9(9)   BINARY.
           02  NUM-BLOCK   PIC S9(9)   BINARY.
           02  I           PIC S9(9)   BINARY.
       01  FLG.
           02  FLG-LDR-EOF PIC 9.
      **************************************************************************
       PROCEDURE           DIVISION.
       000-MAIN                SECTION.
           PERFORM 010-INIT.
           PERFORM 100-INPUT.
           PERFORM
                   UNTIL   FLG-LDR-EOF  NOT =  0
               CALL    LDR-MCP-MODULE  USING
                   LDR-MCPDATA
                   LDR-SPADATA
                   LDR-LINKDATA
                   LDR-SCREENDATA
               MOVE    LDR         TO  LDW
               PERFORM 200-OUTPUT
               PERFORM 100-INPUT
           END-PERFORM.
           PERFORM 090-FINISH.
      *
           STOP    RUN.
      **************************************************************************
       010-INIT                SECTION.
           DISPLAY    '** START MCPMAIN'
               UPON    CONSOLE.
      *
           OPEN    INPUT
               LDR-FILE.
           PERFORM 800-DBOPEN.
           DISPLAY    '** INITIALIZE COMPLETED'
               UPON    CONSOLE.
      **************************************************************************
       090-FINISH              SECTION.
           PERFORM 800-DBDISCONNECT.
           CLOSE
               LDR-FILE.
      *
           DISPLAY    '** END   MCPMAIN'
               UPON    CONSOLE.
      **************************************************************************
       100-INPUT               SECTION.
           DISPLAY    '** WAIT READ'
               UPON    CONSOLE.
      *
           MOVE    ZERO        TO  FLG-LDR-EOF.
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (  I            >  LDR-BLOCKS  )
                       OR  (  FLG-LDR-EOF  >  ZERO        )
               READ    LDR-FILE    INTO    LDR-DATA(I)
                 AT  END
                   MOVE    1           TO  FLG-LDR-EOF
               END-READ
               DISPLAY    '** READ (' I ')'
                   UPON    CONSOLE
           END-PERFORM.
      *
           DISPLAY    '** DONE READ'
               UPON    CONSOLE.
      **************************************************************************
       200-OUTPUT              SECTION.
           OPEN    OUTPUT
               LDW-FILE.
      *
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (  I            >  LDW-BLOCKS  )
               WRITE   LDW-REC     FROM    LDW-DATA(I)
           END-PERFORM.
      *
           CLOSE
               LDW-FILE.
      **************************************************************************
       800-DBOPEN              SECTION.
           MOVE   'DBOPEN'     TO  LDR-MCP-FUNC.
      *
           CALL   'MCPSUB'     USING
                LDR-MCPDATA
                METADB.
      **************************************************************************
       800-DBDISCONNECT        SECTION.
           MOVE   'DBDISCONNECT'    TO  LDR-MCP-FUNC.
      *
           CALL   'MCPSUB'     USING
                LDR-MCPDATA
                METADB.
