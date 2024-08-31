#!/bin/bash

set -xeu
mkdir repo
tar -xvf "$1" -C .

export PYTHONPATH=${PYTHONPATH}:$PWD

#TODO: python -> job environment
python "$2"
