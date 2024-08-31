#!/bin/bash
# This script builds all needed for cuttlefish AppHost servant and upload it into Sandbox as a
# single resource (.tar.gz)

ARCADIA_ROOT=$(readlink -f $(dirname $0)/../../../..)
CUTTLEFISH_ROOT=$ARCADIA_ROOT/alice/cuttlefish
APP_HOST_PORT=3000

CONTENT=(
    $CUTTLEFISH_ROOT/bin/cuttlefish
)

function make() {
    ya make -ttt $1
    if [ $? != 0 ]; then
        exit -1
    fi
}

function upload_cuttlefish_via_ssh() {
    cat $1 | ssh $2 '
        echo -n "Send cuttlefish to the instance..."
        cat - >./cuttlefish-X
        chmod 755 ./cuttlefish-X
        echo "OK"

        echo -n "Stop running cuttlefish..."
        kill -9 `pidof cuttlefish` 1>/dev/null 2>&1
        mv ./cuttlefish-X ./cuttlefish 
        echo "OK"

        echo -n "Wait for cuttlefish process..."
        while ! curl --silent localhost:3000/admin?action=ping >/dev/null; do
            echo -n " ."
            sleep 1
        done
        echo "OK"
    '
}


# -------------------------------------------------------------------------------------------------
for ITEM in ${CONTENT[@]}; do
    if [[ -e $(dirname $ITEM)/ya.make ]]; then
        make $(dirname $ITEM)
    fi
done

if [[ $1 == "--ssh" ]]; then
    TESTING_HOST=${2:-uniproxy2-trunk-2.sas.yp-c.yandex.net}

    echo "Upload cuttlefish binary via SSH onto $TESTING_HOST"
    upload_cuttlefish_via_ssh ${CONTENT[@]} $TESTING_HOST
else
    echo "Upload cuttlefish tar.gz package into Sandbox"
    ya upload --tar --ttl=inf --description VOICE-CUTTLEFISH `realpath ${CONTENT[@]}`
fi


