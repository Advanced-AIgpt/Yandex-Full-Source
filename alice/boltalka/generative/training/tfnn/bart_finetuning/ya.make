PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/training/data/nn/util

    yt/python/client
    nirvana/valhalla/src

    dict/mt/make/libs/common
    dict/mt/make/libs/types
    dict/mt/make/modules/corpus
    dict/mt/make/modules/tfnn

    alice/boltalka/generative/training/data/nn/util
    alice/boltalka/generative/tfnn/bucket_maker/vh

    alice/boltalka/generative/pipelines
)

PY_SRCS(
    experiments/basic.py
    experiments/multi_measurements.py
    __init__.py
    __main__.py
    util.py
)

END()
