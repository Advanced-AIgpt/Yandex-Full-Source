#!/bin/bash

LAYER=$1
INPUT=$2
MODEL=$3
DSSM_BIN="/home/krom/tools/dssm3"

CONTEXT_LEN=$(($(head -n 1 $INPUT | grep -o -P '\t' | wc -l)+1))
CONTEXT_COLS=( "context_2" "context_1" "context_0" )
CONTEXT_COLS=( "${CONTEXT_COLS[@]:${#CONTEXT_COLS[@]}-$CONTEXT_LEN:${#CONTEXT_COLS[@]}}" )
CONTEXT_COLS=$(printf ",%s" "${CONTEXT_COLS[@]}")
CONTEXT_COLS=${CONTEXT_COLS:1}


cat $INPUT | $DSSM_BIN apply -m $MODEL -o $LAYER --header $CONTEXT_COLS --mkl_threads 8
