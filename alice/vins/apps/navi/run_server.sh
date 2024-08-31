VINS_ROOT="$( cd "$(dirname "$0")" ; pwd -P )/../.."
SERVER_BIN="$VINS_ROOT/api/pa/pa"

export VINS_DUMP_REQUESTS=1
export VINS_DJANGO_LOGLEVEL=DEBUG
export VINS_PRELOAD_APP=navi

$SERVER_BIN -b [::]:8001 -w 16 --timeout 18000
