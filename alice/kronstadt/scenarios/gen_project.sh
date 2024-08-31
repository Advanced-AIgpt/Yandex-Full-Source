#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
    PLATFORM_FLAG="--target-platform=default-darwin-arm64"
fi

function get-arcadia-root {
    local R="$DIR"
    while [[ ! -e "${R}/.arcadia.root" ]]; do
        if [[ "${R}" == "/" ]]; then
            echo "$0: must be run from inside Arcadia checkout" >&2
            exit  1
        fi
        R="$(dirname "${R}")"
    done
    echo "$R"
}

SCENARIO=$DIR
ARCADIA_ROOT="$(get-arcadia-root)"

for opt in "$@"; do
  case ${opt} in
    --scenario=*) TMP=${opt#*=} && SCENARIO="$DIR/${TMP#${DIR}/}"
    shift ;;
    --)
    shift ; break ;;
    *) echo Warning: Skipping unknown parameter $opt
    shift ;;
  esac
done

NL=$'\n'
INC_FILE="$DIR/scenarios.inc"
rm $INC_FILE
if [[ $SCENARIO == $DIR ]]; then
    NAME="kronstadt_scenarios"
    SCENARIOS_ABS_PATHS=$(find $DIR/*/ya.make | sed 's!/ya.make!!')
    echo "PEERDIR(" > $INC_FILE && \
    find $DIR/*/ya.make | \
        sed "s!$DIR/!alice/kronstadt/scenarios/!" | \
        sed 's!/ya.make!!' >> $INC_FILE && \
    echo ")" >> $INC_FILE
    ADD_DIVKIT_TO_PROJECT="$ARCADIA_ROOT/divkit/public/json-builder/kotlin"
else
    NAME="$(basename $SCENARIO)_scenario"
    SCENARIOS_ABS_PATHS=$SCENARIO
    echo "PEERDIR(${SCENARIO#$ARCADIA_ROOT/})" > $INC_FILE
fi


KRONSTADT_SCENARIO_INC="${INC_FILE#$ARCADIA_ROOT/}"

echo "Generating project '${NAME}'"

"$DIR"/../../../ya ide idea $PLATFORM_FLAG \
    -DKRONSTADT_SCENARIO_INC=${KRONSTADT_SCENARIO_INC} \
    --project-root="$HOME/IdeaProjects/${NAME}" \
    --local \
    --directory-based \
    --iml-in-project-root \
    --generate-junit-run-configurations \
    --auto-exclude-symlinks \
    --group-modules tree \
    --omit-test-data \
    --auto-exclude-symlinks \
    $SCENARIOS_ABS_PATHS \
    "$ARCADIA_ROOT/alice/kronstadt/shard_runner" \
    "$ARCADIA_ROOT/alice/kronstadt/server" \
    "$ARCADIA_ROOT/alice/paskills/common/apphost-spring-controller" \
    "$ARCADIA_ROOT/alice/paskills/common/apphost-service" \
    "$ARCADIA_ROOT/alice/paskills/common/apphost-http-request" \
    "$ARCADIA_ROOT/alice/kronstadt/core" \
    "$ARCADIA_ROOT/alice/kronstadt/tools" \
    "$ARCADIA_ROOT/alice/library/java" \
    "$ARCADIA_ROOT/alice/divkt_templates" \
    $ADD_DIVKIT_TO_PROJECT \
    "$@"
