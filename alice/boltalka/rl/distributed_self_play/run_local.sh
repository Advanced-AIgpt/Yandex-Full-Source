#!/bin/bash
trap 'killall' INT

killall() {
    trap '' INT TERM     # ignore INT and TERM while shutting down
    echo "**** Shutting down... ****"     # added double quotes
    kill -TERM 0         # fixed order, send TERM not INT
    wait
    echo DONE
}

let port=1337
let port2=$port+1

echo $port $port2

logpid() { while sleep 10; do  ps -p $1 -o pcpu= -o pmem= ; done; }

DATA_DIR=/mnt/storage/nzinov/rl python -m memory_profiler session_sampler.py --master-ip ::1 --master-port $port --master-model-port $port2 &
#logpid $! > sampler.log &
STATE_DIR=/mnt/storage/nzinov/rl/experiments python train.py --master-ip ::1 --master-port $port --master-model-port $port2 --command local_run $@ &
#logpid $! > train.log &

cat
