      ******************************************************************
      * PANDA -- a simple transaction monitor
      *
      * Copyright (C) 1993-1999 Ogochan.
      * 2000-2008 Ogochan & JMARI.
      *
      * This program is free software; you can redistribute it and/or modify
      * it under the terms of the GNU General Public License as published by
      * the Free Software Foundation; either version 2 of the License, or
      * (at your option) any later version.
      *
      * This program is distributed in the hope that it will be useful, but
      * WITHOUT ANY WARRANTY; without even the implied warranty of
      * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
      * General Public License for more details.
      *
      * You should have received a copy of the GNU General Public License
      * along with this program; if not, write to the Free Software
      * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
      ******************************************************************
       IDENTIFICATION DIVISION.
       PROGRAM-ID. MCPMAIN.
       ENVIRONMENT DIVISION.
       CONFIGURATION SECTION.
       SPECIAL-NAMES.
      * CONSOLE IS CONSOLE.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT LDR-FILE
               ASSIGN TO "dc.input"
               ORGANIZATION SEQUENTIAL
               ACCESS MODE SEQUENTIAL.
           SELECT LDW-FILE
               ASSIGN TO "dc.output"
               ORGANIZATION SEQUENTIAL
               ACCESS MODE SEQUENTIAL.
       DATA DIVISION.
       FILE SECTION.
       FD LDR-FILE.
       01 LDR-REC.
           02 FILLER PIC X(1024).
       FD LDW-FILE.
       01 LDW-REC.
           02 FILLER PIC X(1024).
       WORKING-STORAGE SECTION.
       COPY DB-META.
       COPY LDRFILE.
       COPY LDWFILE.
       COPY ENUM-VALUE.
       01 WORK.
           02 WRK-PROG PIC X(16).
           02 WRK-WINEND PIC S9(9) BINARY.
           02 WRK-WINFR PIC S9(9) BINARY.
           02 NUM-BLOCK PIC S9(9) BINARY.
           02 I PIC S9(9) BINARY.
       01 FLG.
           02 FLG-LDR-EOF PIC 9.
      **************************************************************************
       PROCEDURE DIVISION.
       000-MAIN SECTION.
           PERFORM 010-INIT.
           PERFORM 100-INPUT.
           PERFORM
                   UNTIL FLG-LDR-EOF NOT = 0
               CALL LDR-MCP-MODULE USING
                   LDR-MCPDATA
                   LDR-SPADATA
                   LDR-LINKDATA
                   LDR-SCREENDATA
               MOVE LDR TO LDW
               PERFORM 200-OUTPUT
               PERFORM 100-INPUT
           END-PERFORM.
           PERFORM 090-FINISH.
      *
           STOP RUN.
      **************************************************************************
       010-INIT SECTION.
           DISPLAY '** START MCPMAIN'
               UPON CONSOLE.
      *
           OPEN INPUT
               LDR-FILE.
           PERFORM 800-DBOPEN.
           DISPLAY '** INITIALIZE COMPLETED'
               UPON CONSOLE.
      **************************************************************************
       090-FINISH SECTION.
           PERFORM 800-DBDISCONNECT.
           CLOSE
               LDR-FILE.
      *
           DISPLAY '** END   MCPMAIN'
               UPON CONSOLE.
      **************************************************************************
       100-INPUT SECTION.
           DISPLAY '** WAIT READ'
               UPON CONSOLE.
      *
           MOVE ZERO TO FLG-LDR-EOF.
           PERFORM VARYING I FROM 1 BY 1
                   UNTIL ( I > LDR-BLOCKS )
                       OR ( FLG-LDR-EOF > ZERO )
               READ LDR-FILE INTO LDR-DATA(I)
                 AT END
                   MOVE 1 TO FLG-LDR-EOF
               END-READ
               DISPLAY '** READ (' I ')'
                   UPON CONSOLE
           END-PERFORM.
      *
           DISPLAY '** DONE READ'
               UPON CONSOLE.
      **************************************************************************
       200-OUTPUT SECTION.
           OPEN OUTPUT
               LDW-FILE.
      *
           PERFORM VARYING I FROM 1 BY 1
                   UNTIL ( I > LDW-BLOCKS )
               WRITE LDW-REC FROM LDW-DATA(I)
           END-PERFORM.
      *
           CLOSE
               LDW-FILE.
      **************************************************************************
       800-DBOPEN SECTION.
           MOVE 'DBOPEN' TO LDR-MCP-FUNC.
      *
           CALL 'MCPSUB' USING
                LDR-MCPDATA
                METADB.
      **************************************************************************
       800-DBDISCONNECT SECTION.
           MOVE 'DBDISCONNECT' TO LDR-MCP-FUNC.
      *
           CALL 'MCPSUB' USING
                LDR-MCPDATA
                METADB.
