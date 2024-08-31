#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
    PLATFORM_FLAG="--target-platform=default-darwin-arm64"
fi
"$DIR"/../../../ya ide idea $PLATFORM_FLAG --project-root="$HOME/IdeaProjects/dialogovo" \
    --local \
    --directory-based \
    --iml-in-project-root \
    --generate-junit-run-configurations \
    --auto-exclude-symlinks \
    --group-modules tree \
    --omit-test-data \
    --auto-exclude-symlinks \
    "$DIR" \
    "$DIR/../../kronstadt/scenarios/afisha" \
    "$DIR/../../kronstadt/scenarios/alice4business" \
    "$DIR/../../kronstadt/scenarios/automotive_hvac" \
    "$DIR/../../kronstadt/scenarios/contacts" \
    "$DIR/../../kronstadt/scenarios/theremin" \
    "$DIR/../../kronstadt/scenarios/photoframe" \
    "$DIR/../../kronstadt/scenarios/video_call" \
    "$DIR/../../kronstadt/scenarios/skills_discovery" \
    "$DIR/../../kronstadt/scenarios/implicit_skills_discovery" \
    "$DIR/../../kronstadt/scenarios/tv_channels" \
    "$DIR/../../kronstadt/shard_runner" \
    "$DIR/../common/pgconverter" \
    "$DIR/../common/apphost-service" \
    "$DIR/../common/apphost-spring-controller" \
    "$DIR/../common/proto-utils" \
    "$DIR/../common/rest-template-factory" \
    "$DIR/../../../divkit/public/json-builder/kotlin"
