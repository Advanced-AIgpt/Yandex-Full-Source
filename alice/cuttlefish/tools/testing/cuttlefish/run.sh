#!/bin/bash

# set -x

ARCADIA_ROOT=$(readlink -f $(dirname .)/../../../../..)
CUTTLEFISH_BIN=$ARCADIA_ROOT/alice/cuttlefish/bin/cuttlefish/cuttlefish

if ! [ -x $CUTTLEFISH_BIN ]; then
    echo "cuttlefish binary $CUTTLEFISH_BIN doesn't exist, build it!"
    exit 1
fi


rm -rf cuttlefish.evlog cuttlefish.rtlog
$CUTTLEFISH_BIN run -V server.http.apphost_threads=12 -V server.lock_memory=false ${@}
