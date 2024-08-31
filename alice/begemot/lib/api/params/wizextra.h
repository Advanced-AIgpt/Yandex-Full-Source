#pragma once

#include <util/generic/string.h>

namespace NAlice {

    inline const TString WIZEXTRA = "wizextra";

    // Parameters of Alice rules written into parameter 'wizextra'.
    // Format of 'wizextra' value: "key1=value1;key2=value2;key3_without_value;key4_without_value"
    inline const TString WIZEXTRA_KEY_ALICE_NONSENSE_THRESHOLD = "alice_nonsense_threshold";
    inline const TString WIZEXTRA_KEY_ALICE_ORIGINAL_TEXT = "alice_original_text";
    inline const TString WIZEXTRA_KEY_ALICE_PREPROCESSING = "alice_preprocessing";
    inline const TString WIZEXTRA_KEY_DEVICE_ID = "device_id";
    inline const TString WIZEXTRA_KEY_DEVICE_STATE_ACTIONS = "device_actions";
    inline const TString WIZEXTRA_KEY_ACTIVE_SPACE_ACTIONS = "active_space_actions";
    inline const TString WIZEXTRA_KEY_ENABLED_CONDITIONAL_FORMS = "alice_enabled_conditional_forms";
    inline const TString WIZEXTRA_KEY_ENABLED_MEGAMIND_EXPERIMENTS = "alice_enabled_megamind_experiments";
    inline const TString WIZEXTRA_KEY_ENTITIES = "entities";
    inline const TString WIZEXTRA_KEY_GALLERIES = "galleries";
    inline const TString WIZEXTRA_KEY_CONTACTS_PROTO = "contacts_proto";
    inline const TString WIZEXTRA_KEY_GC_MEMORY_STATE = "gc_memory_state";
    inline const TString WIZEXTRA_KEY_IOT_USER_INFO = "iot_user_info";
    inline const TString WIZEXTRA_KEY_IS_SMART_SPEAKER = "is_smart_speaker";
    inline const TString WIZEXTRA_KEY_PREVIOUS_PHRASES = "previous_phrases";
    inline const TString WIZEXTRA_KEY_PREVIOUS_REWRITTEN_REQUESTS = "previous_rewritten_requests";
    inline const TString WIZEXTRA_KEY_RESOLVE_CONTEXTUAL_AMBIGUITY = "resolve_contextual_ambiguity";
    inline const TString WIZEXTRA_KEY_SCENARIO_FRAME_ACTIONS = "available_frame_actions";
    inline const TString WIZEXTRA_KEY_SEMANTIC_FRAME = "semantic_frame";
    inline const TString WIZEXTRA_KEY_USER_ENTITY_DICTS = "user_entity_dicts";
    inline const TString WIZEXTRA_KEY_GRANET_PRINT_SAMPLE_MOCK = "granet_print_sample_mock";
    inline const TString WIZEXTRA_KEY_SEMANTIC_CONTEXT = "semantic_context";

} // namespace NAlice
