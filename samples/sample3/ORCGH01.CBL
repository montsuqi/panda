      *
       IDENTIFICATION          DIVISION.
       PROGRAM-ID.             ORCGH01.
      *****************************************************************
      *  システム名        : ＯＲＣＡ
      *  サブシステム名    : 請求管理
      *  コンポーネント名  : 請求管理−手書き訂正（Ｈ０１）
      *  管理者            : 
      *  作成日付    作業者        記述
      *  00/12/01    MCC−藤原　   新規作成
      *****************************************************************
      *  プログラム修正履歴
      * Maj/Min/Rev  修正者       日付      内容
      *****************************************************************
      *
       ENVIRONMENT             DIVISION.
       CONFIGURATION               SECTION.
       INPUT-OUTPUT                SECTION.
       FILE-CONTROL.
      *
       DATA                    DIVISION.
       FILE                        SECTION.
      *
       WORKING-STORAGE             SECTION.
      *
      *   
           COPY    "ENUM-VALUE".
      *
      *    フラグ領域
       01  FLG-AREA.
           03  FLG-END             PIC 9(01).
           03  FLG-SYSKANRI        PIC 9(01).
           03  FLG-SEIKYU          PIC 9(01).
           03  FLG-PTINF           PIC 9(01).
      *
      *    カウント領域
       01  CNT-AREA.
           03  CNT-ERR             PIC 9(06).
      *
      *    添字領域
       01  IDX-AREA.
           03  IDX                 PIC 9(04).
           03  IDY                 PIC 9(04).
           03  IDZ                 PIC 9(04).
           03  IDX2                PIC 9(04).
      *
      *    一時領域
       01  WRK-AREA.
           03  WRK-HIDMSG          PIC X(70).
      *
           03  WRK-SYO             PIC 9(02).
           03  WRK-AMARI           PIC 9(02).
           03  WRK-WIDGET          PIC 9(02).
      *
           03  WRK-NYUGAIKBN       PIC X(06).
           03  WRK-TEISYUTUSAKI    PIC X(10).
           03  WRK-YM              PIC X(07). 
           03  WRK-SYMD.
               05  WRK-SYY         PIC 9(04).
               05  WRK-SMM         PIC 9(02).
               05  WRK-SDD         PIC 9(02).
           03  WRK-WYMD.
               05  WRK-WGO         PIC X(01).
               05  WRK-WYY         PIC 9(02).
               05  WRK-WMM         PIC 9(02).
               05  WRK-WDD         PIC 9(02).
      *
       01  WRK-HEN-AREA.
           03  WRK-HENYMDG         PIC X(09).
           03  WRK-HENYMD.
               05  WRK-HGO         PIC X(01).
               05  WRK-HYY         PIC 9(02).
               05  FILLER          PIC X(01)   VALUE   ".".
               05  WRK-HMM         PIC 9(02).
               05  FILLER          PIC X(01)   VALUE   ".".
               05  WRK-HDD         PIC 9(02).
      *
                    
      *
      *****************************************************************
      *    ファイルレイアウト
      *****************************************************************
       COPY    DB-ADRS.
      *
      *
      *****************************************************************
      *    連絡　領域
      *****************************************************************
        LINKAGE                     SECTION.
       COPY    MCPAREA.
       COPY    SPAAREA.
       COPY    LINKAREA.
       COPY    SCRAREA.
      *COPY    "H01.INC".
      **************************************************************************
       PROCEDURE           DIVISION   USING
           MCPAREA
           SPAAREA
           LINKAREA
           SCRAREA.
      *
      *****************************************************************
      *    主　　処理
      *****************************************************************
       000-PROC-SEC                SECTION.
      *
           DISPLAY "H01 SPA-MOTOPG = " "[" SPA-MOTOPG "]"
           DISPLAY "H01 MCP-STATUS = " "[" MCP-STATUS "]"
      *
           EVALUATE    MCP-STATUS      ALSO    MCP-EVENT
               WHEN    "LINK"          ALSO    ANY
                   PERFORM 100-INIT-SEC
      *    画面遷移
               WHEN     "PUTG"           ALSO   "CLICKED"
                   PERFORM 200-GMNSENI
                   MOVE "何故でない？" TO  H01-VALUE OF H01-PTNUM
                   MOVE "red"  TO      H01-STYLE OF H01-SRYKANAME
                                       H01-STYLE OF H01-PTNUM
           END-EVALUATE.
      *
           .
           EXIT    PROGRAM
           .
      *****************************************************************
      *    初期　処理
      *****************************************************************
       100-INIT-SEC                SECTION.
      *
           INITIALIZE                  FLG-AREA
           INITIALIZE                  WRK-AREA
           INITIALIZE                  H01
      *
      *
           MOVE    "CURRENT"           TO  MCP-PUTTYPE.
      *    MOVE     SPACE              TO  MCP-PUTTYPE.
           MOVE    "H01    "           TO  MCP-WINDOW.
      *
           PERFORM 900-PUT-WINDOW.
      *
           MOVE    1                   TO  FLG-END
           .
       100-INIT-EXT.
           EXIT.
      *****************************************************************
      *    画面遷移処理
      *****************************************************************
       200-GMNSENI                   SECTION.
      *
           EVALUATE    MCP-EVENT       ALSO    MCP-WIDGET
      *        戻る
               WHEN    "CLICKED"       ALSO    "B01"
                   PERFORM 210-BACK
               WHEN    "CLICKED"       ALSO    "B12"
                   PERFORM 210-KAKUTEI
           END-EVALUATE
      *
           .
       200-GMNSENI-EXT.
           EXIT.
      *****************************************************************
      *    戻る　処理
      *****************************************************************
       210-BACK                    SECTION.
      *
      *
           MOVE   "CHANGE"             TO  MCP-PUTTYPE
           MOVE    'list'              TO  MCP-WINDOW
      *
           PERFORM 900-PUT-WINDOW
      *
           MOVE    1                   TO  FLG-END
      *
           .
       210-BACK-EXT.
           EXIT.
      *
      *****************************************************************
      *    確定　処理
      *****************************************************************
       210-KAKUTEI                    SECTION.
      *
      *
           MOVE    "NEW"                TO  MCP-PUTTYPE
           MOVE    "HID1"               TO  MCP-WINDOW
      *
           PERFORM 900-PUT-WINDOW
      *
           MOVE    1                    TO  FLG-END              
           .
       210-KAKUTEI-EXT.
           EXIT.
      *
      *****************************************************`PN@`PN@ ******
      *    ＰＵＴ　処理
      *****************************************************************
       900-PUT-WINDOW              SECTION.
      *
           MOVE   "PUTWINDOW"       TO  MCP-FUNC.
           CALL   "MCPSUB"   USING
                MCPAREA 
                ADRS.
      *
           .
       900-PUT-WINDOW-EXT.
           EXIT.
      *
      *
