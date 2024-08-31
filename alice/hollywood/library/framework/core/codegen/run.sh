#!/bin/bash

echo "Exporting codegeneration files"
SCRIPT_DIR="$(dirname '$0')"

ya make $SCRIPT_DIR --add-result=.h --add-result=.cpp

cp $SCRIPT_DIR/gen_directives.pb.cpp $SCRIPT_DIR/results/gen_directives.example.cpp
cp $SCRIPT_DIR/gen_directives.pb.h $SCRIPT_DIR/results/gen_directives.example.h
cp $SCRIPT_DIR/gen_server_directives.pb.cpp $SCRIPT_DIR/results/gen_server_directives.example.cpp
cp $SCRIPT_DIR/gen_server_directives.pb.h $SCRIPT_DIR/results/gen_server_directives.example.h
