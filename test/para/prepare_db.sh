#! /bin/bash

. ../pathes

TESTDB='paratest'
TEST1_DB='test1.db'

for c in 1 2 3 4
do
  dropdb $TESTDB$c
done

while test $# != 0
do
  echo $TESTDB$1
  createdb $TESTDB$1
  $DBGEN --create --drop $TEST1_DB | psql $TESTDB$1
  shift
done
