PY3_LIBRARY()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    nirvana/vh3/src
)

RESOURCE(
    resources/collect_pulsar_with_metrics/count_cuts_accuracy.py collect_pulsar_with_metrics/count_cuts_accuracy.py
    resources/collect_pulsar_with_metrics/thresholds_to_model_options_json.py collect_pulsar_with_metrics/thresholds_to_model_options_json.py
    resources/collect_pulsar_with_metrics/slices_to_model_options_json.py collect_pulsar_with_metrics/slices_to_model_options_json.py
    resources/collect_pulsar_with_metrics/columns_result.json collect_pulsar_with_metrics/columns_result.json

    resources/data_preparation/filter_features_post_win_reason.yql data_preparation/filter_features_post_win_reason.yql
    resources/data_preparation/sample_join_reqid_35.yql data_preparation/sample_join_reqid_35.yql
    resources/data_preparation/join_marks_features.yql data_preparation/join_marks_features.yql
    resources/data_preparation/sample_three_reqid_joins.yql data_preparation/sample_three_reqid_joins.yql
    resources/data_preparation/sample_join_reqid_80.yql data_preparation/sample_join_reqid_80.yql

    resources/measure_quality_test/deduplication.yql measure_quality_test/deduplication.yql
    resources/measure_quality_test/reformat_from_catboost.py measure_quality_test/reformat_from_catboost.py

    resources/postclassifier_learning_filter_factors/reformat_from_catboost.py postclassifier_learning_filter_factors/reformat_from_catboost.py

    resources/prepostclassifier_learning/reformat_from_catboost.py prepostclassifier_learning/reformat_from_catboost.py

    resources/preclassifier_thresholds/merge_predicts.py preclassifier_thresholds/merge_predicts.py
    resources/preclassifier_thresholds/thresholds_selection.py preclassifier_thresholds/thresholds_selection.py
    resources/preclassifier_thresholds/measure_quality.py preclassifier_thresholds/measure_quality.py

    resources/resolve_slices/model_metadata.py resolve_slices/model_metadata.py
    resources/resolve_slices/ignored_features.py resolve_slices/ignored_features.py
)

PY_SRCS(
    __init__.py
    classifiers_training.py
    collect_pulsar_with_metrics.py
    collect_resources_with_formulas.py
    data_preparation.py
    enums.py
    measure_quality_test.py
    operations.py
    mm_preclassifier_emulation.py
    postclassifier_learning_filter_factors.py
    preclassifier_thresholds.py
    queries.py
    resolve_slices.py
    eval_feature.py
    eval_feature_module.py
)

END()
