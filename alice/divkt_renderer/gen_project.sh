#!/usr/bin/env bash
PREFIX="idea(divkt_renderer):"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ACTION_NAME='ya idea';ACTION_START=`date +%s`;echo -e "$PREFIX starting $ACTION_NAME"
if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
    PLATFORM_FLAG="--target-platform=default-darwin-arm64"
fi
"$DIR"/../../ya ide idea ${YA_COMMON_OPTS} ${YA_JDK_OPTS} $PLATFORM_FLAG --project-root="$HOME/IdeaProjects/divkt_renderer" --directory-based --iml-in-project-root --generate-junit-run-configurations --local \
    $DIR/. \
    $DIR/../paskills/common/apphost-service \
    $DIR/../paskills/common/apphost-spring-controller

echo -e "$PREFIX finished $ACTION_NAME in $(($(date +%s) - $ACTION_START)) seconds"
