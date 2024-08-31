#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

"$DIR"/../../shard_runner/start_shard.sh --scenario-path=$DIR $@
