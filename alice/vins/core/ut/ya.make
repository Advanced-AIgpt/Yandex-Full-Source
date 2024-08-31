PY2TEST()

OWNER(g:alice)

IF(AUTOCHECK)
    TIMEOUT(600)

    SIZE(MEDIUM)

    REQUIREMENTS(
        network:full
        cpu:4 ram:9
    )

    FORK_SUBTESTS()

    SPLIT_FACTOR(100)

ELSE()
    TIMEOUT(10000)

    TAG(ya:not_autocheck)

ENDIF()

PEERDIR(alice/vins/core/test_lib)

DEPENDS(alice/vins/resources)

SRCDIR(alice/vins/core)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

TEST_SRCS(
    vins_core/test/__init__.py
    vins_core/test/conftest.py
    vins_core/test/test_alice_type_parser.py
    vins_core/test/test_anaphora.py
    vins_core/test/test_annotations.py
    vins_core/test/test_combine_scores_classifier.py
    vins_core/test/test_config.py
    vins_core/test/test_custom_entities.py
    vins_core/test/test_dialog_history.py
    vins_core/test/test_entitysearch_http_api.py
    vins_core/test/test_features.py
    vins_core/test/test_flow_nlu.py
    vins_core/test/test_form_filler.py
    vins_core/test/test_fst.py
    vins_core/test/test_general_conversation.py
    vins_core/test/test_http_base.py
    vins_core/test/test_http_base_proxy.py
    vins_core/test/test_irrelevant_classifier.py
    vins_core/test/test_log.py
    vins_core/test/test_metric_learning.py
    vins_core/test/test_metrics.py
    vins_core/test/test_microintent_storage.py
    vins_core/test/test_nlu_postprocessor.py
    vins_core/test/test_pb_serialization.py
    vins_core/test/test_protocol_semantic_frame_classifier.py
    vins_core/test/test_protocol_semantic_frame_tagger.py
    vins_core/test/test_pynorm.py
    vins_core/test/test_realtime_cls.py
    vins_core/test/test_request.py
    vins_core/test/test_reranker.py
    vins_core/test/test_response.py
    vins_core/test/test_s3.py
    vins_core/test/test_sample_processors.py
    vins_core/test/test_samples.py
    vins_core/test/test_sk_http_api.py
    vins_core/test/test_slots_map_utils.py
    vins_core/test/test_syntax.py
    vins_core/test/test_template_nlg.py
    vins_core/test/test_token_tagger.py
    vins_core/test/test_updater.py
    vins_core/test/test_validation.py

    # slowtest
    vins_core/test/test_dialog_manager.py
    vins_core/test/test_rnn_tagger.py
    vins_core/test/test_session.py
    vins_core/test/test_token_classifier.py
    vins_core/test/test_archive.py
    vins_core/test/test_crossvalidation.py
)

END()
