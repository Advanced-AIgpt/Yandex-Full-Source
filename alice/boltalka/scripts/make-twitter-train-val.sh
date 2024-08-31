#!/bin/bash
FILE=$1
tail -n 65411 ~/datasets/vins/$FILE > ~/datasets/vins/$FILE.val
head -n -65411 ~/datasets/vins/$FILE | shuf --random-source=<(./random-bytes.sh) > ~/datasets/vins/$FILE.train
