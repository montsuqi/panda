            02  H01.
              03  H01-PTNUM.
                04  H01-VALUE   PIC X(21).
              03  H01-SRYYM.
                04  H01-VALUE   PIC X(8).
              03  H01-SRYKA.
                04  H01-SRYKA-STATE   PIC S9(9)   BINARY.
                04  H01-SRYKA-VALUE   PIC X(3).
              03  H01-MIDASI-G  OCCURS  35.
                04  H01-MIDASI-STYLE   PIC X(20).
                04  H01-MIDASI         PIC X(18).
              03  H01-KHNTEN-G  OCCURS  35.
                04  H01-KHNTEN-STATE   PIC S9(9)   BINARY.
                04  H01-KHNTEN   PIC 9(8).
              03  H01-KAISU-G   OCCURS  35.
                04  H01-KAISU-STATE   PIC S9(9)   BINARY.
                04  H01-KAISU   PIC 9(5).
              03  H01-TENSU-G   OCCURS  3.
                04  H01-TENSU-TBL OCCURS  35.
                  05  H01-TENSU-STATE   PIC S9(9)   BINARY.
                  05  H01-TENSU   PIC 9(8).
              03  H01-TOTALTEN-G  OCCURS  3.
                04  H01-TOTALTEN-STATE   PIC S9(9)   BINARY.
                04  H01-TOTALTEN   PIC 9(8).
              03  H01-FTNMONEY-G  OCCURS  3.
                04  H01-FTNMONEY-STATE   PIC S9(9)   BINARY.
                04  H01-FTNMONEY   PIC 9(8).
              03  H01-YKZFTN-G  OCCURS  3.
                04  H01-YKZFTN-STATE   PIC S9(9)   BINARY.
                04  H01-YKZFTN   PIC 9(8).
              03  H01-JNISSU-G  OCCURS  3.
                04  H01-JNISSU-STATE   PIC S9(9)   BINARY.
                04  H01-JNISSU   PIC 9(2).
              03  H01-NKHNTEN-G  OCCURS  35.
                04  H01-NKHNTEN   PIC 9(8).
              03  H01-NKAISU-G  OCCURS  35.
                04  H01-NKAISU   PIC 9(5).
              03  H01-NTENSU-G  OCCURS  3.
                04  H01-NTENSU-TBL  OCCURS  35.
                  05  H01-NTENSU   PIC 9(8).
              03  H01-NTOTALTEN-G OCCURS  3.
                04  H01-NTOTALTEN-STATE   PIC S9(9)   BINARY.
                04  H01-NTOTALTEN   PIC 9(8).
              03  H01-NFTNMONEY-G OCCURS  3.
                04  H01-NFTNMONEY   PIC 9(8).
              03  H01-NYKZFTN-G   OCCURS  3.
                04  H01-NYKZFTN   PIC 9(8).
              03  H01-NJNISSU-G   OCCURS  3.
                04  H01-NJNISSU   PIC 9(2).
              03  H01-SKYKBN.
                04  H01-VALUE   PIC 9(1).
              03  H01-TEISEIKBN.
                04  H01-VALUE   PIC 9(1).
              03  H01-SKYYM.
                04  H01-VALUE   PIC X(8).
              03  H01-HENREIYM.
                04  H01-VALUE   PIC X(8).
              03  H01-SRYKANAME.
                04  H01-SRYKANAME-STYLE   PIC X(20).
                04  H01-SRYKANAME-VALUE   PIC X(18).
              03  H01-NAME.
                04  H01-NAME-STYLE   PIC X(20).
                04  H01-NAME-VALUE   PIC X(25).
              03  H01-SEIKYUSYUBETU.
                04  H01-SEIKYUSYUBETU-STYLE   PIC X(20).
                04  H01-SEIKYUSYUBETU-VALUE   PIC X(50).
              03  H01-SKYKBNNM.
                04  H01-SKYKBNNM-STYLE   PIC X(20).
                04  H01-SKYKBNNM-VALUE   PIC X(12).
              03  H01-TEISEIKBNNM.
                04  H01-TEISEIKBNNM-STYLE   PIC X(20).
                04  H01-TEISEIKBNNM-VALUE   PIC X(12).
