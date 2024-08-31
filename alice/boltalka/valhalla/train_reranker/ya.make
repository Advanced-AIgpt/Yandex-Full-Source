PY2_PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    contrib/python/PyYAML

    yt/python/client
    nirvana/valhalla/src
    alice/boltalka/valhalla
    alice/boltalka/valhalla/lib/pulsar
    alice/boltalka/valhalla/train_reranker/factors
    alice/boltalka/valhalla/train_reranker/reporting
    alice/boltalka/valhalla/train_reranker/seq2seq
    alice/boltalka/valhalla/lib/pulsar
)

END()

RECURSE(
    factors
    reporting
    seq2seq
)
