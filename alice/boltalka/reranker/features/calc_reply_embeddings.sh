#!/bin/bash

LAYER=$1
INPUT=$2
MODEL=$3
DSSM_BIN="/home/krom/tools/dssm3"

cat $INPUT | $DSSM_BIN apply -m $MODEL -o $LAYER --header reply --mkl_threads 8
