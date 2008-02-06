       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     TEST1.
      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 2008 Ogochan
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
      *   System Name      : MONTSUQI
      *   Subsystem Name   : database test
      *   Component Name   : COBOL API test
      *   Author           : ogochan@wasp-corp.co.jp
      ******************************************************************
       ENVIRONMENT         DIVISION.
       CONFIGURATION           SECTION.
       SPECIAL-NAMES.
           CONSOLE         IS  CONSOLE.
       DATA                DIVISION.
       WORKING-STORAGE         SECTION.
       01  FLG.
           02  FLG-EOF     PIC 9.
       01  I           PIC S9(9)   BINARY.
      *
       COPY    MCPAREA.
       COPY    DB-META.
       01  DB-TEST1.
           02  DB-TEST1-REC   OCCURS 100.
             COPY    DB-TEST1.
       LINKAGE                 SECTION.
       01  PARA.
           02  FILLER      PIC X(256).
      **************************************************************************
       PROCEDURE           DIVISION
               USING
           PARA.
       000-MAIN                SECTION.
           DISPLAY    '*** TEST1'
               UPON    CONSOLE.
      *
           PERFORM 010-INIT.
           DISPLAY    '*** INIT'
               UPON    CONSOLE.
           PERFORM 100-INSERT.
           PERFORM 200-SELECT-FETCH1.
           PERFORM 300-SELECT-FETCH2.
           PERFORM 400-SELECT.
           PERFORM 090-FINISH.
      *
           STOP    RUN.
      **************************************************************************
       010-INIT                SECTION.
           PERFORM 800-DBOPEN.
           PERFORM 800-DBSTART.
      **************************************************************************
       090-FINISH              SECTION.
           PERFORM 800-DBCOMMIT.
           PERFORM 800-DBDISCONNECT.
      **************************************************************************
       100-INSERT              SECTION.
           DISPLAY    '*** INSERT START'
               UPON    CONSOLE.
      *
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   I  >  100
               MOVE    I           TO  TST-ID(1)
               MOVE    1           TO  TST-CLASS(1)
               MOVE    I           TO  TST-CODE(1)
               PERFORM 800-DBINSERT-TEST1
           END-PERFORM.
      *
           DISPLAY    '*** INSERT END'
               UPON    CONSOLE.
      **************************************************************************
       200-SELECT-FETCH1       SECTION.
           DISPLAY    '*** SELECT & FETCH(1) START'
               UPON    CONSOLE.
      *
           MOVE   '100'        TO  TST-ID(1).
           PERFORM 800-DBSELECT-TEST1-ID2.
           PERFORM
                   UNTIL   MCP-RC  NOT =  ZERO
               PERFORM 800-DBFETCH-TEST1-ID2
               IF      MCP-RC  =  ZERO
                   DISPLAY TST-ID(1) ' ' TST-CLASS(1) ' ' TST-CODE(1)
                       UPON    CONSOLE
               END-IF
           END-PERFORM.
           PERFORM 800-DBCLOSECURSOR-TEST1-ID2.
      **************************************************************************
       300-SELECT-FETCH2       SECTION.
           DISPLAY    '*** SELECT & FETCH(2) START'
               UPON    CONSOLE.
      *
           MOVE   '100'        TO  TST-ID(1).
           PERFORM 800-DBSELECT-TEST1-ID2.
           PERFORM
                   UNTIL   MCP-RC  NOT =  ZERO
               MOVE    10          TO  MCP-LIMIT
               PERFORM 800-DBFETCH-TEST1-ID2
               IF      MCP-RC  =  ZERO
                   PERFORM VARYING I   FROM    1   BY  1
                           UNTIL   I  >  MCP-RCOUNT
                       DISPLAY I ':' TST-ID(I) ' ' TST-CLASS(I) ' '
                               TST-CODE(I)
                           UPON    CONSOLE
                   END-PERFORM
               END-IF
           END-PERFORM.
           PERFORM 800-DBCLOSECURSOR-TEST1-ID2.
      **************************************************************************
       400-SELECT              SECTION.
           DISPLAY    '*** SELECT START'
               UPON    CONSOLE.
      *
           MOVE   '100'        TO  TST-ID(1).
           MOVE    20          TO  MCP-LIMIT
           PERFORM 800-DBSELECT-TEST1-ID3.
           IF      MCP-RC  =  ZERO
               PERFORM VARYING I   FROM    1   BY  1
                       UNTIL   I  >  MCP-RCOUNT
                   DISPLAY I ':' TST-ID(I) ' ' TST-CLASS(I) ' '
                           TST-CODE(I)
                       UPON    CONSOLE
               END-PERFORM
           END-IF
           PERFORM 800-DBCLOSECURSOR-TEST1-ID2.
      **************************************************************************
       800-DBOPEN              SECTION.
           DISPLAY    '*** DBOPEN'
               UPON    CONSOLE.
           MOVE    LOW-VALUE   TO  MCP-PATH.
           MOVE   'DBOPEN'     TO  MCP-FUNC.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                METADB.
           DISPLAY    '*** DBOPEN END'
               UPON    CONSOLE.
      **************************************************************************
       800-DBSTART             SECTION.
           MOVE    LOW-VALUE   TO  MCP-PATH.
           MOVE   'DBSTART'    TO  MCP-FUNC.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                METADB.
      **************************************************************************
       800-DBINSERT-TEST1      SECTION.
           MOVE   'DBINSERT'   TO  MCP-FUNC.
           MOVE   'test1'      TO  MCP-TABLE.
           MOVE   'id'         TO  MCP-PATHNAME.
      *
           DISPLAY TST-ID(1) ' ' TST-CLASS(1) ' ' TST-CODE(1)
               UPON    CONSOLE.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                TST(1).
      **************************************************************************
       800-DBSELECT-TEST1-ID2  SECTION.
           MOVE   'DBSELECT'   TO  MCP-FUNC.
           MOVE   'test1'      TO  MCP-TABLE.
           MOVE   'id2'        TO  MCP-PATHNAME.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                TST(1).
      **************************************************************************
       800-DBSELECT-TEST1-ID3  SECTION.
           MOVE   'DBSELECT'   TO  MCP-FUNC.
           MOVE   'test1'      TO  MCP-TABLE.
           MOVE   'id3'        TO  MCP-PATHNAME.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                DB-TEST1.
      **************************************************************************
       800-DBFETCH-TEST1-ID2   SECTION.
           MOVE   'DBFETCH'    TO  MCP-FUNC.
           MOVE   'test1'      TO  MCP-TABLE.
           MOVE   'id2'        TO  MCP-PATHNAME.
           INITIALIZE  DB-TEST1.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                TST(1).
      **************************************************************************
       800-DBCLOSECURSOR-TEST1-ID2     SECTION.
           MOVE   'DBCLOSECURSOR'      TO  MCP-FUNC.
           MOVE   'test1'      TO  MCP-TABLE.
           MOVE   'id2'        TO  MCP-PATHNAME.
      *
           CALL   'MCPSUB'     USING
                MCPAREA
                TST(1).
      **************************************************************************
       800-DBCOMMIT            SECTION.
           MOVE    LOW-VALUE   TO  MCP-PATH.
           MOVE   'DBCOMMIT'   TO  MCP-FUNC.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                METADB.
      **************************************************************************
       800-DBDISCONNECT        SECTION.
           MOVE    LOW-VALUE   TO  MCP-PATH.
           MOVE   'DBDISCONNECT'    TO  MCP-FUNC.
      *
           CALL   'MONFUNC'    USING
                MCPAREA
                METADB.
