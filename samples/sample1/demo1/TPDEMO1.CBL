       IDENTIFICATION      DIVISION.
       PROGRAM-ID.     TPDEMO1.
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
      *   コンポーネント名：デモ1
      *   管理者          ：ogochan@NetLab.jp
      *   日付日付  作業者  記述
      *   00.11.17  ....    修正内容
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
       01  J           PIC S9(9)   BINARY.
       COPY    ENUM-VALUE.
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
           DISPLAY    '*** TPDEMO1'
               UPON    CONSOLE.
      *
           DISPLAY    'STATUS [' MCP-STATUS ']'
               UPON    CONSOLE.
           IF      MCP-STATUS  =  'PUTG'
               DISPLAY    '*** PAGE = '    WN1-NOTEBOOK1-PAGE
                   UPON    CONSOLE
           END-IF.
           EVALUATE    MCP-STATUS      ALSO    MCP-EVENT
             WHEN     'LINK'           ALSO    ANY
               PERFORM 010-INIT
             WHEN     'PUTG'           ALSO   'OpenCalc'
               PERFORM 210-OPEN-CALC
             WHEN     'PUTG'           ALSO   'CloseCalc'
               PERFORM 220-CLOSE-CALC
             WHEN     'PUTG'           ALSO   'PutData'
               PERFORM 230-PUT-DATA
             WHEN     'PUTG'           ALSO   'Left'
               PERFORM 240-CLICK-LEFT
             WHEN     'PUTG'           ALSO   'Right'
               PERFORM 250-CLICK-RIGHT
             WHEN     'PUTG'           ALSO   'OpenCalendar'
               PERFORM 260-OPEN-CALENDAR
             WHEN     'PUTG'           ALSO   'Quit'
               PERFORM 270-QUIT
             WHEN      OTHER
               PERFORM 290-OTHER
           END-EVALUATE.
      *
           EXIT    PROGRAM.
      **************************************************************************
       010-INIT                SECTION.
           DISPLAY 'LNK-LINKTEXT =[' LNK-LINKTEXT ']'
               UPON    CONSOLE.
      *
           IF          (  LNK-LINKTEXT  =  SPACE  )
                   OR  (  LNK-LINKTEXT  =  LOW-VALUE  )
               MOVE   '漢字を入れてみた'   TO  WN1-ENTRY1-VALUE
               MOVE    ZERO        TO  SPA-COUNT
             ELSE
               MOVE    LNK-LINKTEXT        TO  WN1-ENTRY1-VALUE
           END-IF.
           MOVE   WIDGET-INSENSITIVE   TO  WN1-ENTRY1-STATE.
      *
           MOVE   'green'      TO  WN1-ENTRY1-STYLE.
           MOVE   'red'        TO  WN1-ENTRY2-STYLE.
           MOVE   'blue'       TO  WN1-ENTRY3-STYLE.
      *
           MOVE    -1234       TO  WN1-ENTRY2-VALUE.
           MOVE   'おごちゃん' TO  WN1-COMBO1-ITEM(1).
           MOVE   'えりさ'     TO  WN1-COMBO1-ITEM(2).
           MOVE   'ques'       TO  WN1-COMBO1-ITEM(3).
           MOVE   'blue'       TO  WN1-COMBO1-STYLE.
           MOVE    3           TO  WN1-COMBO1-COUNT.
      *
           MOVE   'おごちゃん' TO  WN1-LIST1-ITEM(1).
           MOVE   'えりさ'     TO  WN1-LIST1-ITEM(2).
           MOVE   'ques'       TO  WN1-LIST1-ITEM(3).
           MOVE   'void_No2'   TO  WN1-LIST1-ITEM(4).
           MOVE   'Mul6'       TO  WN1-LIST1-ITEM(5).
           MOVE   '末広'       TO  WN1-LIST1-ITEM(6).
           MOVE    6           TO  WN1-LIST1-COUNT.
           MOVE    2           TO  WN1-LIST1-FROM.
           MOVE   'T'          TO  WN1-LIST1-SELECT(2).
      *
           MOVE   'その１'     TO  WN1-CLIST1-LABEL1-VALUE.
           MOVE   'blue'       TO  WN1-CLIST1-LABEL1-STYLE.
           MOVE   'その２'     TO  WN1-CLIST1-LABEL2-VALUE.
           MOVE   'red'        TO  WN1-CLIST1-LABEL2-STYLE.
           MOVE   'その３'     TO  WN1-CLIST1-LABEL3-VALUE.
           MOVE   'green'      TO  WN1-CLIST1-LABEL3-STYLE.
           MOVE    2           TO  WN1-CLIST1-FROM.
      *
           MOVE   'まだ'       TO  WN1-TOGGLEBUTTON1L.
           MOVE   'green'      TO  WN1-TOGGLEBUTTON1S.
      *
           MOVE   'F'          TO  WN1-TOGGLEBUTTON1.
      *
           MOVE   'F'          TO  WN1-CHECKLEFT.
           MOVE   'F'          TO  WN1-CHECKRIGHT.
           MOVE   'F'          TO  WN1-CHECKNONE.
      *
           MOVE   'T'          TO  WN1-RADIOLEFT.
           MOVE   'F'          TO  WN1-RADIORIGHT.
           MOVE   'F'          TO  WN1-RADIONONE.
      *
           MOVE    3           TO  WN1-NOTEBOOK1-PAGE.
      *
           MOVE   'ノートブックの中身'
                               TO  WN1-TEXT1-VALUE.
           MOVE    1234        TO  WN1-ENTRY4-VALUE.
      *
           MOVE   '名前'       TO  WN1-CLIST2-LABEL1.
           MOVE   '電話番号'   TO  WN1-CLIST2-LABEL2.
           MOVE   'おごちゃん' TO  WN1-CLIST2-VALUE1(1).
           MOVE   '070-6163-7932'
                               TO  WN1-CLIST2-VALUE2(1).
           MOVE    1           TO  WN1-CLIST2-COUNT.
      *
           MOVE   'item1'      TO  WN1-OPTION1-ITEM(1).
           MOVE   'item2'      TO  WN1-OPTION1-ITEM(2).
           MOVE   'item3'      TO  WN1-OPTION1-ITEM(3).
           MOVE   'item4'      TO  WN1-OPTION1-ITEM(4).
           MOVE    4           TO  WN1-OPTION1-COUNT.
           MOVE    2           TO  WN1-OPTION1-SELECT.
      *
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'entry2'     TO  MCP-WIDGET.
           MOVE   'project1'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       210-OPEN-CALC           SECTION.
           MOVE   'NEW'        TO  MCP-PUTTYPE.
           MOVE   'project2'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       220-CLOSE-CALC          SECTION.
           MOVE   'CLOSE'      TO  MCP-PUTTYPE.
           MOVE   'project2'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       230-PUT-DATA            SECTION.
           DISPLAY '*'
               UPON    CONSOLE.
           DISPLAY 'OPTION =' WN1-OPTION1-SELECT
               UPON    CONSOLE.
           DISPLAY '* SPA-COUNT = ' SPA-COUNT
               UPON    CONSOLE
      *    IF      WN1-ENTRY4-VALUE  >  ZERO
               DISPLAY 'WN1-ENTRY4-VALUE = ' "[" 
                       WN1-ENTRY4-VALUE "]"
      *    END-IF.
           IF      WN1-TOGGLEBUTTON1  =  'T'
               COMPUTE SPA-COUNT = SPA-COUNT + 1
               MOVE    WN1-ENTRY1-VALUE
                   TO  WN1-CLIST1-VALUE1(SPA-COUNT)
               MOVE    WN1-ENTRY2-VALUE-X
                   TO  WN1-CLIST1-VALUE2(SPA-COUNT)
               MOVE    WN1-ENTRY1-VALUE
                   TO  WN1-CLIST1-VALUE3(SPA-COUNT)
               MOVE    SPA-COUNT   TO  WN1-CLIST1-COUNT
               MOVE   '選んだ'     TO  WN1-TOGGLEBUTTON1L
             ELSE
               MOVE   '選んでない' TO  WN1-TOGGLEBUTTON1L
           END-IF.
      *
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   I  >  20
               IF      WN1-CLIST1-SELECT(I)  =  'T'
                   MOVE   'よい子'     TO  WN1-CLIST1-VALUE1(I)
               END-IF
           END-PERFORM.
      *
           MOVE   '*'          TO  WRK-BUFF.    
           IF      WN1-CHECKNONE  =  'T'
               IF      WN1-CHECKLEFT  =  'T'
                   IF      WN1-CHECKRIGHT =  'T'
                       MOVE   'なし左右'   TO  WRK-BUFF
                     ELSE
                       MOVE   'なし左'     TO  WRK-BUFF
                   END-IF
                 ELSE
                   IF      WN1-CHECKRIGHT =  'T'
                       MOVE   'なし右'     TO  WRK-BUFF
                     ELSE
                       MOVE   'なし'       TO  WRK-BUFF
                   END-IF
               END-IF
             ELSE
               IF      WN1-CHECKLEFT  =  'T'
                   IF      WN1-CHECKRIGHT =  'T'
                       MOVE   '左右'   TO  WRK-BUFF
                     ELSE
                       MOVE   '左'     TO  WRK-BUFF
                   END-IF
                 ELSE
                   IF      WN1-CHECKRIGHT =  'T'
                       MOVE   '右'     TO  WRK-BUFF
                   END-IF
               END-IF
           END-IF.
           EVALUATE    TRUE
             WHEN      WN1-RADIOLEFT   =  'T'
               STRING  WRK-BUFF
                       WN1-ENTRY1-VALUE
                       WN1-ENTRY2-VALUE-X
                   DELIMITED   BY  SPACE
                   INTO    WRK-BUFF
             WHEN      WN1-RADIORIGHT  =  'T'
               STRING  WRK-BUFF
                       WN1-ENTRY2-VALUE-X
                       WN1-ENTRY1-VALUE
                   DELIMITED   BY  SPACE
                   INTO    WRK-BUFF
             WHEN      OTHER
               CONTINUE
           END-EVALUATE.
           MOVE    WRK-BUFF    TO  WN1-ENTRY3-VALUE.
           PERFORM VARYING I   FROM    1   BY  1
                   UNTIL   (  I  >  20  )
               IF      WN1-LIST1-SELECT(I)  =  'T'
                   MOVE   'よい子'     TO  WN1-LIST1-ITEM(I)
                   MOVE   'F'          TO  WN1-LIST1-SELECT(I)
               END-IF
           END-PERFORM.
      *
      *    MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'project1'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       240-CLICK-LEFT          SECTION.
           STRING
                   WN1-ENTRY1-VALUE
                   WN1-ENTRY2-VALUE-X
               DELIMITED   BY  SPACE
               INTO    WN1-ENTRY3-VALUE.
           MOVE   'F'          TO  WN1-CHECKLEFT.
      *
      *    MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'project1'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       250-CLICK-RIGHT         SECTION.
           STRING
                   WN1-ENTRY2-VALUE-X
                   WN1-ENTRY1-VALUE
               DELIMITED   BY  SPACE
               INTO    WN1-ENTRY3-VALUE.
           MOVE   'F'          TO  WN1-CHECKRIGHT.
      *
      *    MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'project1'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       260-OPEN-CALENDAR       SECTION.
           MOVE   'CHANGE'     TO  MCP-PUTTYPE.
           MOVE   'project5'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       270-QUIT                SECTION.
           MOVE   WN1-ENTRY1-VALUE TO  LNK-LINKTEXT.
      *
           MOVE   'CHANGE'     TO  MCP-PUTTYPE.
           MOVE   'project3'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       290-OTHER               SECTION.
      *    MOVE   'CURRENT'    TO  MCP-PUTTYPE.
           MOVE    SPACE       TO  MCP-PUTTYPE.
           MOVE   'project1'   TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      **************************************************************************
       900-PUT-WINDOW          SECTION.
           MOVE   'PUTWINDOW'  TO  MCP-FUNC.
           CALL   'MCPSUB'     USING
                MCPAREA
                METADB.
