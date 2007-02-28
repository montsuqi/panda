#! /bin/sh

PANDA=/home/ogochan/MONTSUQI/panda-current
MONITOR=$PANDA/tools/monitor
HTSERVER=$PANDA/htserver/htserver

export APS_LOAD_PATH=.
export APS_HANDLER_LOAD_PATH=.:$PANDA/aps/.libs
export MONPS_LOAD_PATH=.:$PANDA/libs/.libs:$PANDA/monpls_demo/.libs

#$MONITOR -restart -allrestart -wait 1 -cache 100 -sesdir mon&
$MONITOR -no-aps-retry -wait 1 -cache 100 -sesdir mon&

echo $MONPS_LOAD_PATH

$HTSERVER -panda /tmp/web.term -port /tmp/web.htserver
