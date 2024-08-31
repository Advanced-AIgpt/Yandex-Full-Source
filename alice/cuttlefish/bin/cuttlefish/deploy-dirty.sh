#!/bin/bash

LOCAL_PATH=$(readlink -f $(dirname $0))/cuttlefish
TARGET_PATH="cuttlefish"
BACKUP_PATH="cuttlefish.backup"

_CMD_RESTART="
    PID=\$(pidof cuttlefish)

    echo 'Kill running cuttlefish...'
    kill \$PID
    while kill -0 \$PID; do
        sleep 0.3
    done

    while ! curl --fail http://localhost:4000/admin?action=ping >/dev/null 2>&1; do
        echo '...wait for ready...'
        sleep 1
    done
    echo 'Cuttlefish is ready'
"

function deploy() {
    echo "DEPLOY \"$LOCAL_PATH\" ONTO \"$HOSTNAME\"" >&2

    if ! scp $LOCAL_PATH $HOSTNAME:$TARGET_PATH.X; then
        echo "Could scp \"$LOCAL_PATH\" to \"$HOSTNAME:$TARGET_PATH.X\"" >&2
        exit 1
    fi

    ssh $HOSTNAME "
        if [ ! -e $BACKUP_PATH ]; then
            if cp $TARGET_PATH $BACKUP_PATH && chown loadbase:loadbase $BACKUP_PATH; then
                echo 'Backup is made'
            else
                exit 1
            fi
        else
            echo 'Backup already exists'
        fi

        chown loadbase:loadbase $TARGET_PATH.X
        chmod 775 $TARGET_PATH.X
        mv --force $TARGET_PATH.X $TARGET_PATH

        echo 'Going to restart cuttlefish...'
        $_CMD_RESTART
    "
}

function rollback() {
     ssh $HOSTNAME "
        mv --force $BACKUP_PATH $TARGET_PATH
        $_CMD_RESTART
    "
}

function print_help() {
    echo "Tool to quick'n'dirty deploy and launch localy built binary onto remote host.
Usage: $0 {deploy|rollback} <target hostname>" >&2
}


CMD=$1
HOSTNAME=$2

case $CMD in
help) print_help;;
deploy) deploy "$@";;
rollback) rollback "$@";;
*) echo "Unknown command \"$CMD\"" >&2 && print_help && exit 1;;
esac

