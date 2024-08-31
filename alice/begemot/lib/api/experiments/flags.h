#pragma once

#include <util/generic/string.h>

namespace NAlice {

    // Begemot experiments.
    // Experiments are written as keys of cgi-parameter 'wizextra'.
    inline const TString EXP_BEGEMOT_PREFIX = "bg_";

    // Enable fast data for all rules
    inline const TString EXP_BEGEMOT_FRESH_ALICE = "bg_fresh_alice";
    inline const TString EXP_BEGEMOT_FRESH_ALICE_ENTITY = "bg_fresh_alice_entity";
    inline const TString EXP_BEGEMOT_FRESH_ALICE_FORM = "bg_fresh_alice_form";
    inline const TString EXP_BEGEMOT_FRESH_ALICE_PREFIX = "bg_fresh_alice_prefix";

    // Enable fast data only for granet
    inline const TString EXP_BEGEMOT_FRESH_GRANET = "bg_fresh_granet";
    inline const TString EXP_BEGEMOT_FRESH_GRANET_ENTITY = "bg_fresh_granet_entity";
    inline const TString EXP_BEGEMOT_FRESH_GRANET_FORM = "bg_fresh_granet_form";
    inline const TString EXP_BEGEMOT_FRESH_GRANET_PREFIX = "bg_fresh_granet_prefix";

    inline const TString EXP_BEGEMOT_FRAME_AGGREGATOR_CONFIG_PATCH = "bg_frame_aggregator_config_patch";
    inline const TString EXP_BEGEMOT_FRAME_AGGREGATOR_CONFIG_PATCH_BASE64 = "bg_frame_aggregator_config_patch_base64";
    inline const TString EXP_BEGEMOT_DISABLE_FRAME = "bg_disable_frame";
    inline const TString EXP_BEGEMOT_ALICE_TRIVIAL_TAGGER_LOG = "bg_alice_trivial_tagger_log";
    inline const TString EXP_BEGEMOT_ALICE_TRIVIAL_TAGGER_CONFIG_PATCH = "bg_alice_trivial_tagger_config_patch";
    inline const TString EXP_BEGEMOT_ALICE_TRIVIAL_TAGGER_CONFIG_PATCH_BASE64 = "bg_alice_trivial_tagger_config_patch_base64";
    inline const TString EXP_BEGEMOT_ALICE_BINARY_INTENT_CLASSIFIER_LOG = "bg_alice_binary_intent_classifier_log";
    inline const TString EXP_BEGEMOT_DUMP_GRANET_DATA = "bg_dump_granet_data";
    inline const TString EXP_BEGEMOT_DUMP_GRANET_DATA_BRIEF = "bg_dump_granet_data_brief";
    inline const TString EXP_BEGEMOT_GRANET_LOG = "bg_granet_log";
    inline const TString EXP_BEGEMOT_GRANET_SOURCE_TEXT = "bg_granet_source_text";
    inline const TString EXP_BEGEMOT_IOT_LOG = "bg_iot_log";
    inline const TString EXP_BEGEMOT_LSTM_CLASSIFIED_INTENT = "bg_lstm_classified_intent";
    inline const TString EXP_BEGEMOT_NEW_NONSENSE_ENTITY_BUILDER = "bg_new_nonsense_entity_builder";
    inline const TString EXP_BEGEMOT_REWRITE_ANAPHORA = "bg_rewrite_anaphora";
    inline const TString EXP_BEGEMOT_REWRITE_ELLIPSIS = "bg_rewrite_ellipsis";
    inline const TString EXP_BEGEMOT_ENABLE_GRANET_FOR_WIZARD = "bg_enable_granet_for_wizard";
    inline const TString EXP_BEGEMOT_ENABLE_MULTI_INTENT_CLASSIFIER = "bg_enable_multi_intent_classifier";
    inline const TString EXP_BEGEMOT_ENABLE_SEARCH_QUERY_NORMALIZATION = "bg_enable_search_query_normalization";
    inline const TString EXP_BEGEMOT_SEARCH_QUERY_ANAPHORA_REWRITE_THRESHOLD = "bg_search_query_anaphora_rewrite_threshold";
    inline const TString EXP_BEGEMOT_ENABLE_SHOW_ROUTE_ON_BINARY_BEGGINS_CLASSIFIER = "bg_show_route_on_binary_beggins_classifier";
    inline const TString EXP_BEGEMOT_CONTACTS_SKIP_EXACT = "bg_contacts_skip_exact";
    inline const TString EXP_BEGEMOT_ALICE_CUSTOM_ENTITIES_STATIC_ENTITIES_RESULT = "bg_alice_custom_entities_static_entities_result";
    inline const TString EXP_BEGEMOT_ALICE_CUSTOM_ENTITIES_ADD_THESAURUS_SYNONYMS = "bg_alice_custom_entities_add_thesaurus_synonyms";
    inline const TString EXP_BEGEMOT_GRANET_DISABLE_CONTACTS = "bg_granet_disable_contacts";
    inline const TString EXP_BEGEMOT_ENABLE_DEV_CLASSIFIER = "bg_enable_dev_classifier";
    inline const TString EXP_BEGEMOT_ENUMERATE_DEV_CLASSIFIERS = "bg_enumerate_dev_classifiers";

    inline const TString EXP_BEGEMOT_POLYGLOT_RESPONSE_MERGER_CONFIG_PATCH = "bg_polyglot_response_merger_config_patch";
    inline const TString EXP_BEGEMOT_POLYGLOT_RESPONSE_MERGER_CONFIG_PATCH_BASE64 = "bg_polyglot_response_merger_config_patch_base64";
    inline const TString EXP_BEGEMOT_POLYGLOT_RESPONSE_MERGER_FORCE_FRAME_MERGE_MODE = "bg_polyglot_response_merger_force_frame_merge_mode";
    inline const TString EXP_BEGEMOT_POLYGLOT_RESPONSE_MERGER_LOG = "bg_polyglot_response_merger_log";

    // Product
    inline const TString EXP_BEGEMOT_PASKILLS = "bg_paskills";
    inline const TString EXP_BEGEMOT_SNEZHANA = "bg_snezhana";

    inline const TString EXP_FRAMES_OVERRIDE_RULE_THRESHOLD_PREFIX = "bg_frames_override_rule_threshold=";

    inline const TString EXP_BEGEMOT_GC_CLASSIFIER_CONTEXT_LENGTH = "bg_gc_classifier_context_length";

    inline const TString EXP_BEGEMOT_ITEM_SELECTOR_NAME = "bg_item_selector_name";
} // namespace NAlice
