#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
docker pull registry.yandex.net/rtc-base/xenial:stable
"$DIR"/../../../../ya package "$DIR/pkg.json" --docker --docker-repository=paskills $@
