#!/usr/bin/env bash

PORT="$1"
shift

while [ -n "$1" ]
do
    case "$1" in
        [+!]windows_fixlist.tar.gz)
            tar xzf windows_fixlist.tar.gz -C data/
            curl -s "http://localhost:${PORT}/admin/update_yastroka_fixlist"
        ;;
        [+!]bno.trie)
            mv -f bno.trie data/
            curl -s "http://localhost:${PORT}/admin/update_bno_apps"
        ;;
    esac
    shift
done
