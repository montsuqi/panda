       IDENTIFICATION              DIVISION.
       PROGRAM-ID.                 MCPSUB.
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
      ******************************************************************
      *  システム名        : ＯＲＣＡ
      *  サブシステム名    : 共通
      *  コンポーネント名  : サブ
      *  管理者            : 
      *  作成日付    作業者        記述
      *  XX.XX.XX    ＮＮＮ        新規作成
      ******************************************************************
       ENVIRONMENT                 DIVISION.
       CONFIGURATION               SECTION.
       INPUT-OUTPUT            SECTION.
       FILE-CONTROL.
           SELECT  DBR-FILE
               ASSIGN  TO  "db.input"
               ORGANIZATION    SEQUENTIAL
               ACCESS  MODE    SEQUENTIAL.
           SELECT  DBW-FILE
               ASSIGN  TO  "db.output"
               ORGANIZATION    SEQUENTIAL
               ACCESS  MODE    SEQUENTIAL.
           SELECT  LOG-FILE
               ASSIGN  TO  "log.output"
               ORGANIZATION    SEQUENTIAL
               ACCESS  MODE    SEQUENTIAL.
       DATA                        DIVISION.
       FILE                    SECTION.
       FD  DBR-FILE.
       01  DBR-REC.
           02  FILLER      PIC X(1024).
       FD  DBW-FILE.
       01  DBW-REC.
           02  FILLER      PIC X(1024).
       FD  LOG-FILE.
       01  LOG-REC.
           02  FILLER      PIC X(1024).
       WORKING-STORAGE         SECTION.
       01  WORK.
           02  I           PIC S9(9)   BINARY.
           02  J           PIC S9(9)   BINARY.
       01  FLG.
           02  FLG-DBR-EOF PIC 9.
       01  PATH-NULL.
           02  FILLER      PIC S9(9)   BINARY  VALUE 1.
           02  FILLER      PIC S9(9)   BINARY  VALUE -1.
           02  FILLER      PIC S9(9)   BINARY  VALUE -1.
      *
           COPY    DBCOMM.
       LINKAGE                     SECTION.
           COPY    MCPAREA.
           COPY    DBAREA.
       PROCEDURE                   DIVISION
           USING   MCPAREA
                   DBAREA.
      *****************************************************************
       000-ENTRY               SECTION.
           PERFORM 010-DUMP-PARA.
      *
           EVALUATE    MCP-FUNC
             WHEN     'DBOPEN'
               PERFORM 100-DBOPEN
             WHEN     'PUTWINDOW'
               PERFORM 300-PUTWINDOW
             WHEN     'DBDISCONNECT'
               PERFORM 900-DBCLOSE
             WHEN     'DBCLOSE'
               PERFORM 900-DBCLOSE
             WHEN     'DBSTART'
               PERFORM 110-DB-OPERATION
             WHEN     'DBCOMMIT'
               PERFORM 110-DB-OPERATION
               CONTINUE
             WHEN      OTHER
               PERFORM 200-TABLE-OPERATION
           END-EVALUATE.
      *
           EXIT    PROGRAM.
      *****************************************************************
       010-DUMP-PARA           SECTION.
           DISPLAY '** MCP-FUNC     [' MCP-FUNC ']'
               UPON    SYSOUT.
           DISPLAY '** MCP-RC       [' MCP-RC ']'
               UPON    SYSOUT.
           DISPLAY '** MCP-BLOCKS   [' MCP-BLOCKS ']'
               UPON    SYSOUT.
           DISPLAY '** MCP-RNAME    [' MCP-RNAME ']'
               UPON    SYSOUT.
           DISPLAY '** MCP-PNAME    [' MCP-PNAME ']'
               UPON    SYSOUT.
      *****************************************************************
       100-DBOPEN              SECTION.
           OPEN    INPUT
               DBR-FILE.
      *
           PERFORM 110-DB-OPERATION.
      *****************************************************************
       110-DB-OPERATION        SECTION.
           MOVE    MCP-FUNC    TO  DBCOMM-FUNC.
           MOVE    PATH-NULL   TO  DBCOMM-PATH.
      *
           PERFORM 210-SEND-SQL.
           PERFORM 220-RECV-ANS.
      *****************************************************************
       200-TABLE-OPERATION     SECTION.
           MOVE    DBDATA      TO  DBCOMM-RECORD.
           MOVE    MCP-FUNC    TO  DBCOMM-FUNC.
           MOVE    MCP-PATH    TO  DBCOMM-PATH.
      *
           PERFORM 210-SEND-SQL.
           PERFORM 220-RECV-ANS.
      *****************************************************************
       210-SEND-SQL            SECTION.
           OPEN    OUTPUT
      *        LOG-FILE
               DBW-FILE.
      *
           IF      (  DBCOMM-PATH-BLOCKS  =  ZERO  )
               MOVE    1           TO  DBCOMM-PATH-BLOCKS
           END-IF.
      *
           PERFORM VARYING J   FROM    1   BY  1
                   UNTIL   (  J    >  DBCOMM-PATH-BLOCKS  )
               WRITE   DBW-REC     FROM    DBCOMM-BLOCK(J)
      *        WRITE   LOG-REC     FROM    DBCOMM-BLOCK(J)
           END-PERFORM.
      *
           CLOSE
      *        LOG-FILE
               DBW-FILE.
      *****************************************************************
       220-RECV-ANS            SECTION.
           MOVE    ZERO        TO  FLG-DBR-EOF.
           PERFORM VARYING J   FROM    1   BY  1
                   UNTIL   (  J    >  DBCOMM-PATH-BLOCKS  )
                       OR  (  FLG-DBR-EOF  >  ZERO        )
               READ    DBR-FILE    INTO    DBCOMM-BLOCK(J)
                 AT  END
                   MOVE    1           TO  FLG-DBR-EOF
               END-READ
           END-PERFORM.
      *
           MOVE    DBCOMM-RECORD   TO  DBDATA.
           MOVE    DBCOMM-RC       TO  MCP-RC.
      *****************************************************************
       300-PUTWINDOW           SECTION.
           MOVE   'PUTG'   TO  MCP-STATUS.
           MOVE    ZERO    TO  MCP-RC.
      *****************************************************************
       900-DBCLOSE             SECTION.
           PERFORM 110-DB-OPERATION.
      *
           CLOSE
               DBR-FILE.
