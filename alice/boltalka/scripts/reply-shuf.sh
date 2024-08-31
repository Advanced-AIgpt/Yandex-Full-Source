#!/bin/bash
FILE=$1
cat $FILE | awk -v FS='\t' '{ print $NF }' | paste - $FILE | sort -S1G -Rs -k1,1 --random-source=<(./random-bytes.sh) | cut -f2-
