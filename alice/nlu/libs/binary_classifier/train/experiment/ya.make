PY2_PROGRAM(binary_classifier_trainer)

OWNER(dan-anastasev)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/core
    alice/vins/tools/vins_tools
    contrib/python/scikit-learn
    contrib/python/tqdm
    yt/python/client
)

COPY_FILE(
    alice/vins/core/vins_core/utils/intent_renamer.py
    intent_renamer.py
)

PY_SRCS(
    __main__.py
    dataset_utils.py
    dump_data_for_lstm.py
    intent_renamer.py
    prepare_test_data.py
)

END()
