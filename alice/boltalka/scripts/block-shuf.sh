#!/bin/bash
FILE=$1
CONTEXT=$2
(( DIALOG_SIZE = CONTEXT * 30 ))
(( FIRST_DIALOG_SIZE = DIALOG_SIZE - CONTEXT * (CONTEXT - 1) / 2 ))
cat $FILE | awk -v OFS='\t' -v dsize=$DIALOG_SIZE -v fdsize=$FIRST_DIALOG_SIZE \
    '{ print NR <= fdsize ? 0 : int((NR - fdsize - 1) / dsize + 1), $0 }' | sort -S2G -Rs -k1,1 --random-source=<(./random-bytes.sh) | cut -f2-
