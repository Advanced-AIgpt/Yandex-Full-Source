#!/bin/bash
set -e

GIT_COMMIT=$(git rev-parse HEAD)
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
DEVELOP_REPORTS="develop_reports"

S3_PATH="pa_nlu_quality"
ERROR_STATS="error_stats"

function upload_report_files {
    python tools/nlu/nlu_reports.py upload \
        "activations.intent.metrics.json" \
        "activations-request.intent.metrics.json" \
        "checklist.intent.metrics.json" \
        "pa_player.intent.metrics.json" \
        "scenarios.intent.metrics.json" \
        "search-or-converse.intent.metrics.json" \
        "$S3_PATH/$1"
}

function download_report_files {
    DIR=$1
    OUTDIR=$2

    python tools/nlu/nlu_reports.py download \
        "$S3_PATH/$DIR/activations.intent.metrics.json" \
        "$S3_PATH/$DIR/activations-request.intent.metrics.json" \
        "$S3_PATH/$DIR/checklist.intent.metrics.json" \
        "$S3_PATH/$DIR/pa_player.intent.metrics.json" \
        "$S3_PATH/$DIR/scenarios.intent.metrics.json" \
        "$S3_PATH/$DIR/search-or-converse.intent.metrics.json" \
        "$OUTDIR"
}

function compare_report {
    REPORT=$1
    echo "Comparing $DEVELOP_REPORTS/$REPORT and $REPORT"
    python tools/nlu/compare_reports.py compare "$DEVELOP_REPORTS/$REPORT" "$REPORT" \
    --error-stats="$ERROR_STATS" "${@:2}"
}

function compare_report_files {
    echo "Start compare reports"
    compare_report "activations.intent.metrics.json" -d 0.04
    compare_report "activations-request.intent.metrics.json" -d 0.03
    compare_report "checklist.intent.metrics.json" -m personal_assistant.scenarios.search -d 0.1
    compare_report "pa_player.intent.metrics.json" -d 0.01
    compare_report "scenarios.intent.metrics.json" -d 0.01 \
        -m personal_assistant.scenarios.find_poi \
        -m personal_assistant.scenarios.get_weather \
        -m personal_assistant.scenarios.music_play \
        -m personal_assistant.scenarios.video_play
    compare_report "search-or-converse.intent.metrics.json" -d 0.01
    python tools/nlu/compare_reports.py report_and_exit "$ERROR_STATS"
}

upload_report_files $GIT_COMMIT

if [[ "$GIT_BRANCH" == "develop" ]]; then
    upload_report_files "develop"
else
    if [ -d "$DEVELOP_REPORTS" ]; then
        rm -rf "$DEVELOP_REPORTS"
    fi
    mkdir "$DEVELOP_REPORTS"

    download_report_files "develop" "$DEVELOP_REPORTS"
    compare_report_files
fi
