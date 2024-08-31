PY2_PROGRAM()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    yt/python/client
    library/python/resource
)

SRCDIR(
    alice/vins/core/vins_core/utils
)

PY_SRCS(
    biases.py
    MAIN main.py
    TOP_LEVEL intent_renamer.py
)

RESOURCE(
    alice/vins/apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_for_vins.json /toloka_intent_renames_for_vins
)

END()
