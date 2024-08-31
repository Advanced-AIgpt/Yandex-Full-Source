# use PREFIX variable to put models under specific directory
# if you add new model also add ${PREFIX} before added paths and add files in MODEL_FILES variable

FROM_SANDBOX(
    3335375988
    RENAME
    model.mmap
    model_description
    intents_mapping.json
    OUT
    ${PREFIX}lstm_models/multilabel_intent_classifier/model.mmap
    ${PREFIX}lstm_models/multilabel_intent_classifier/model_description
    ${PREFIX}lstm_models/multilabel_intent_classifier/intents_mapping.json
)

SET(
    MODEL_FILES
    ${PREFIX}lstm_models/multilabel_intent_classifier/model.mmap
    ${PREFIX}lstm_models/multilabel_intent_classifier/model_description
    ${PREFIX}lstm_models/multilabel_intent_classifier/intents_mapping.json
)
