#!/bin/bash

CUTTLEFISH_DIR=$(dirname $(readlink -f $0))
LOG=$CUTTLEFISH_DIR/package.log

ya package -ttt -r $CUTTLEFISH_DIR/cuttlefish-package.json 2>&1 | tee $LOG

VER=$(tail -1 $LOG | sed 's/^.*: //')
PACKAGE_FILE=cuttlefish.$VER.tar.gz

if ! [ -e $PACKAGE_FILE ]; then
    echo "Could not find package: $PACKAGE_FILE" >&2
    exit -1
fi

mv $PACKAGE_FILE $CUTTLEFISH_DIR/cuttlefish.tar.gz
ya upload --type=VOICETECH_CUTTLEFISH_PACKAGE --ttl=14 --description="Cuttlefish package ($VER)" $CUTTLEFISH_DIR/cuttlefish.tar.gz
