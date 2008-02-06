#! /bin/bash

. ./conf

TEST1_DB='test1.db'

dropdb $TESTDB
createdb $TESTDB

$DBGEN --create --drop $TEST1_DB | psql $TESTDB

