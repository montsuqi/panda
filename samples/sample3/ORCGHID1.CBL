      *
       IDENTIFICATION                            DIVISION.
       PROGRAM-ID.                               ORCGHID1.
      *****************************************************************
      *  システム名        : ＯＲＣＡ
      *  サブシステム名    : 請求管理
      *  コンポーネント名  : 確認画面１（ＲＩＤ１）
      *  管理者            : 
      *  作成日付    作業者        記述
      *  00/12/01    MCC−藤原　   新規作成
      *****************************************************************
      *  プログラム修正履歴
      * Maj/Min/Rev  修正者       日付      内容
      *****************************************************************
      *
       ENVIRONMENT                               DIVISION.
       CONFIGURATION                             SECTION.
       INPUT-OUTPUT                              SECTION.
       FILE-CONTROL.
      *
       DATA                                      DIVISION.
      *FILE                                      SECTION.
      *
      *
       WORKING-STORAGE                           SECTION.
      *
      *    フラグ領域
       01  FLG-AREA.
           03  FLG-END                           PIC 9(01).
      *
      *****************************************************************
      *    ファイルレイアウト
      *****************************************************************
       COPY    DB-ADRS.
      *
      *
      *****************************************************************
      *    サブプロ用　領域
      *****************************************************************
      *
      *****************************************************************
      *    連絡　領域
      *****************************************************************
        LINKAGE                                  SECTION.
      *
       COPY    MCPAREA.
       COPY    SPAAREA.
      *
           COPY    "LINKAREA".
      *
       COPY    SCRAREA.

      *
       PROCEDURE                                 DIVISION    USING
                                                 MCPAREA
                                                 SPAAREA
                                                 LINKAREA
                                                 SCRAREA.
      *
      *****************************************************************
      *    主　　処理
      *****************************************************************
       000-PROC-SEC                              SECTION.
      *
      * 
           EVALUATE    MCP-STATUS                ALSO    MCP-EVENT
               WHEN    "LINK"                    ALSO    ANY
                   PERFORM 100-INIT-SEC
      *    画面遷移
               WHEN     "PUTG"                   ALSO   "CLICKED"
                   PERFORM 200-GMNSENI
           END-EVALUATE.
      *
      *
           EXIT    PROGRAM.
      *****************************************************************
      *    初期　処理
      *****************************************************************
       100-INIT-SEC                              SECTION.
      *
           MOVE     SPACE              TO  MCP-PUTTYPE.
           MOVE    "HID1    "          TO  MCP-WINDOW.
           PERFORM 900-PUT-WINDOW.
           .
       100-INIT-EXT.
           EXIT.
      *
      *****************************************************************
      *    画面遷移処理
      *****************************************************************
       200-GMNSENI                               SECTION.
      *
           EVALUATE    MCP-EVENT                 ALSO    MCP-WIDGET
      *        戻る
               WHEN    "CLICKED"                 ALSO    "B01"
                   PERFORM 210-BACK
      *     登録
               WHEN    "CLICKED"                 ALSO    "B12"
                   PERFORM 210-BACK
           END-EVALUATE
      *
           .
       200-GMNSENI-EXT.
           EXIT.
      *****************************************************************
      *    戻る　処理
      *****************************************************************
       210-BACK                                  SECTION.
      *
           MOVE    "HID1"                        TO  SPA-MOTOPG
      *
           MOVE    "list"                        TO  MCP-WINDOW.
           MOVE    "JOIN"                        TO  MCP-PUTTYPE.
      *
           PERFORM 900-PUT-WINDOW
      *
           MOVE    1                             TO  FLG-END.
      *
       210-BACK-EXT.
           EXIT.
      *
      *****************************************************************
      *    ＰＵＴ　処理
      *****************************************************************
       900-PUT-WINDOW                            SECTION.
      *
           MOVE   "PUTWINDOW"                    TO  MCP-FUNC.
           CALL   "MCPSUB"                       USING
                                                 MCPAREA
                                                 ADRS.
      *
       900-PUT-WINDOW-EXT.
           EXIT.
      *
