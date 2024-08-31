PY2_PROGRAM(binary_classifier_trainer)

OWNER(dan-anastasev)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/core
    alice/vins/tools/vins_tools
    contrib/python/click
    yt/python/client
    nirvana/valhalla/src
)

PY_SRCS(
    __init__.py
    dataset_utils.py
    dssm_based_binary_classifier.py
    global_options.py
    MAIN main.py
    prepare_nlu_data.py
    train.py
    vh_collect_embeddings_subgraph.py
    vh_train_subgraph.py
)

END()

RECURSE(
    collect_begemot_responses
)
