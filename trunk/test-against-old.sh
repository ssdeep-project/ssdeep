#!/bin/bash

COUNT=0
FAIL=0
for file in `find /usr/bin /usr/include /usr/local/bin /bin $HOME/Documents`
  do
    TEST1=`./ssdeep-2.9  $file 2>/dev/null | md5deep`
    TEST2=`./ssdeep      $file 2>/dev/null | md5deep`
    if [ $TEST1 != $TEST2 ]; then
      echo "$file: No match"
      FAIL=$(($FAIL+1))
    fi
  COUNT=$(($COUNT+1))
  done
SUCCESS=$(($COUNT-$FAIL))
echo "$COUNT files tested, $SUCCESS passed, $FAIL failed."
