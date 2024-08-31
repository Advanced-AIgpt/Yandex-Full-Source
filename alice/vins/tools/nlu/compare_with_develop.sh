#!/bin/bash
set -e

GIT_COMMIT=$(git rev-parse HEAD)
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
DEVELOP_REPORTS="develop_reports"
RESULTS_DIR=.
SCRIPT_DIR=`dirname $0`

S3_PATH="pa_nlu_quality"
ERROR_STATS="error_stats"

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
function upload_report_files {
    python tools/nlu/nlu_reports.py upload \
        "$RESULTS_DIR/activations.intent.metrics.json" \
        "$RESULTS_DIR/activations-request.intent.metrics.json" \
        "$RESULTS_DIR/checklist.intent.metrics.json" \
        "$RESULTS_DIR/pa_player.intent.metrics.json" \
        "$RESULTS_DIR/scenarios.intent.metrics.json" \
        "$RESULTS_DIR/search-or-converse.intent.metrics.json" \
        "$RESULTS_DIR/quasar_lsr.intent.metrics.json" \
        "$S3_PATH/$1"
}

if [ -d "$DEVELOP_REPORTS" ]; then
    rm -rf "$DEVELOP_REPORTS"
fi
mkdir "$DEVELOP_REPORTS"
download_report_files "develop" "$DEVELOP_REPORTS"

# now we can finally compare the files
$SCRIPT_DIR/compare_base.sh "$DEVELOP_REPORTS" "$RESULTS_DIR"

# if we are on the develop branch, we upload the files in order to make future comparison easier
if [[ "$GIT_BRANCH" == "develop" ]]; then
    upload_report_files "develop"
fi
