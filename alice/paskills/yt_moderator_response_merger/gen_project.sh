#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
    PLATFORM_FLAG="--target-platform=default-darwin-arm64"
fi
"$DIR"/../../../ya ide idea $PLATFORM_FLAG --project-root="$HOME/IdeaProjects/yt_moderator_response_merger" --local --directory-based --iml-in-project-root --generate-junit-run-configurations
