#!/bin/sh
DBSTUB=/home/kida/panda-0.9.11/aps/dbstub
 echo "echo1 = " $1
$DBSTUB -db ORCA  -bd demo7 SAMPLEC
