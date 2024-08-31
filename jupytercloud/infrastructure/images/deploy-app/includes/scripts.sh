#!/usr/bin/env bash

_lineno() { wc -l "$1" | awk '{ print $1 }'; }

log_speed () {
  if (($# < 1)); then
    cat <<HELP
Check the speed of your logs in lines per second.
usage: log_speed <log_path> <period (default: 5)>
HELP
  return 1
  fi

  local log="$1"
  local period="${2:-5}"
  local old; old=$(_lineno "$log")
  local new=$old

  while true; do
    printf "%(%T)T\t %3s/s\t%10s total\n" \
           -1 "$(( new/period - old/period ))" "$new"
    sleep "$period"
    old=$new
    new=$(_lineno "$log")
  done;
}
