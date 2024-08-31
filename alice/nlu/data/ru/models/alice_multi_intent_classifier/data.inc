# use PREFIX variable to put models under specific directory
# if you add new model also add ${PREFIX} before added paths and add files in MODEL_FILES variable

FROM_SANDBOX(
    2242366415
    RENAME
    toloka_word_lstm/model.mmap
    toloka_word_lstm/model_description
    toloka_word_lstm/intents_mapping.json
    OUT
    ${PREFIX}lstm_models/generic_scenarios/model.mmap
    ${PREFIX}lstm_models/generic_scenarios/model_description
    ${PREFIX}lstm_models/generic_scenarios/intents_mapping.json
)

FROM_SANDBOX(
    2868288515
    RENAME
    model.mmap
    model_description
    intents_mapping.json
    OUT
    ${PREFIX}lstm_models/generic_scenarios_v2_quasar/model.mmap
    ${PREFIX}lstm_models/generic_scenarios_v2_quasar/model_description
    ${PREFIX}lstm_models/generic_scenarios_v2_quasar/intents_mapping.json
)

SET(
    MODEL_FILES
    ${PREFIX}lstm_models/generic_scenarios/model.mmap
    ${PREFIX}lstm_models/generic_scenarios/model_description
    ${PREFIX}lstm_models/generic_scenarios/intents_mapping.json
    ${PREFIX}lstm_models/generic_scenarios_v2_quasar/model.mmap
    ${PREFIX}lstm_models/generic_scenarios_v2_quasar/model_description
    ${PREFIX}lstm_models/generic_scenarios_v2_quasar/intents_mapping.json
)
