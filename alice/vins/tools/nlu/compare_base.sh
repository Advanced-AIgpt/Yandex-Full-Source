#!/bin/bash
BASELINE_DIR=$1
RESULTS_DIR=$2

ERROR_STATS="error_stats"

# we assume that we already have pickled results of classification into the RESULTS_DIR:
# activations-request.output.pkl
# activations.output.pkl
# checklist.pkl
# nlu.output.pkl
# pa_player.pkl
# quasar_lsr.pkl
# we also assume that we have the corresponding *metric.json files in the RESULTS_DIR

echo "Measure scenarios"
if [[ -f $RESULTS_DIR/nlu.output.pkl ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/nlu.output.pkl \
        --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json \
        --errors --other=.*other --baseline $BASELINE_DIR/scenarios.intent.metrics.json
    mv $RESULTS_DIR/nlu.output.intent.errors $RESULTS_DIR/scenarios.intent.errors
    mv $RESULTS_DIR/nlu.output.intent.metrics.json $RESULTS_DIR/scenarios.intent.metrics.json
fi

echo "Measure search-or-converse on the Search release dataset"
if [[ -f $RESULTS_DIR/search_release.pkl ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/search_release.pkl \
        --rename apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_search_converse.json \
        --errors --baseline $BASELINE_DIR/mobile-search-or-converse.intent.metrics.json
    mv $RESULTS_DIR/search_release.intent.errors $RESULTS_DIR/mobile-search-or-converse.intent.errors
    mv $RESULTS_DIR/search_release.intent.metrics.json $RESULTS_DIR/mobile-search-or-converse.intent.metrics.json
fi

echo "Measure search-or-converse on the Quasar release dataset"
if [[ -f $RESULTS_DIR/quasar_lsr.pkl ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/quasar_lsr.pkl \
        --rename apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_search_converse.json \
        --errors --baseline $BASELINE_DIR/quasar-search-or-converse.intent.metrics.json
    mv $RESULTS_DIR/quasar_lsr.intent.errors $RESULTS_DIR/quasar-search-or-converse.intent.errors
    mv $RESULTS_DIR/quasar_lsr.intent.metrics.json $RESULTS_DIR/quasar-search-or-converse.intent.metrics.json
fi

echo "Measure skill activations"
if [[ -f "$RESULTS_DIR/activations.output.pkl" ]]; then
  ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/activations.output.pkl --errors \
    --baseline $BASELINE_DIR/activations.intent.metrics.json
  mv $RESULTS_DIR/activations.output.intent.errors $RESULTS_DIR/activations.intent.errors
  mv $RESULTS_DIR/activations.output.intent.metrics.json $RESULTS_DIR/activations.intent.metrics.json
fi

echo "Measure skill activations with requests"
if [[ -f $RESULTS_DIR/activations-request.output.pkl ]]; then
  ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/activations-request.output.pkl --errors \
    --baseline $BASELINE_DIR/activations-request.intent.metrics.json
  mv $RESULTS_DIR/activations-request.output.intent.errors $RESULTS_DIR/activations-request.intent.errors
  mv $RESULTS_DIR/activations-request.output.intent.metrics.json $RESULTS_DIR/activations-request.intent.metrics.json
fi

echo "Measure player commands"
if [[ -f "$RESULTS_DIR/pa_player.pkl" ]]; then
  ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/pa_player.pkl --errors \
    --baseline $BASELINE_DIR/pa_player.intent.metrics.json
fi

echo "Measure scenarios intents checklist"
if [[ -f "$RESULTS_DIR/checklist.pkl" ]]; then
  ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/checklist.pkl --errors \
  --baseline $BASELINE_DIR/checklist.intent.metrics.json
fi

echo "Measure Search release dataset"
if [[ -f "$RESULTS_DIR/search_release.pkl" ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/search_release.pkl \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json \
    --errors --other=.*other --baseline $BASELINE_DIR/search_release.intent.metrics.json
fi

echo "Measure Auto release dataset"
if [[ -f "$RESULTS_DIR/auto_release.pkl" ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/auto_release.pkl \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_auto.json \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json \
    --errors --other=.*other --baseline $BASELINE_DIR/auto_release.intent.metrics.json
fi

echo "Measure Navi release dataset"
if [[ -f "$RESULTS_DIR/navi_release.pkl" ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/navi_release.pkl \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_auto.json \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json \
    --errors --other=.*other --baseline $BASELINE_DIR/navi_release.intent.metrics.json
fi

echo "Measure quasar LSR dataset"
if [[ -f "$RESULTS_DIR/quasar_lsr.pkl" ]]; then
    ./tools/nlu/nlu_tools process_nlu_on_dataset personal_assistant report $RESULTS_DIR/quasar_lsr.pkl \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_quasar.json \
    --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json \
    --errors --other=.*other --baseline $BASELINE_DIR/quasar_lsr.intent.metrics.json
fi

# compare p-values with critical values
if [[ -f $ERROR_STATS ]]; then
  rm $ERROR_STATS
fi
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/scenarios.intent.metrics.json -s 30 -p 0.01 --error-stats $ERROR_STATS
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/search-or-converse.intent.metrics.json -s 30 -p 0.01 --error-stats $ERROR_STATS
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/activations.intent.metrics.json -s 30 -p 0.01 --error-stats $ERROR_STATS
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/activations-request.intent.metrics.json -s 30 -p 0.01 --error-stats $ERROR_STATS
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/pa_player.intent.metrics.json -s 10 -p 0.01 --error-stats $ERROR_STATS
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/checklist.intent.metrics.json -s 10 -p 0.01 --error-stats $ERROR_STATS
./tools/nlu/nlu_tools compare_reports compare_p_values $RESULTS_DIR/quasar_lsr.intent.metrics.json -s 30 -p 0.01 --error-stats $ERROR_STATS

./tools/nlu/nlu_tools compare_reports report_and_exit "$ERROR_STATS"
