PY3_PROGRAM()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    nirvana/vh3/src
    alice/quality/classifier_graph/graph
    library/python/nirvana
)

RESOURCE(
    ../config/quasar.json quasar.json
    ../config/touch.json touch.json
)

PY_SRCS(MAIN gen.py)

END()
