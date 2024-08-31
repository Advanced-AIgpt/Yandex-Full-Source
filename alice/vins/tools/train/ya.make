PY2_PROGRAM(train_tools)

OWNER(g:alice)

PEERDIR(
    alice/vins/core
    alice/vins/tools/vins_tools
    alice/vins/apps/personal_assistant
    contrib/python/click
    alice/vins/apps/crm_bot
)

PY_SRCS(
    __main__.py
    compile_model_from_resources.py
    data_loaders.py
    dataset.py
    dataset_building.py
    split_model_into_resources.py
    train_metric_learning.py
    train_taggers.py
    train_utils.py
    vins_config_tools.py
)

END()

RECURSE (
    build_reranker_dataset
    tagger
)
