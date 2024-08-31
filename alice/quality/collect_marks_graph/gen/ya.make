PY3_PROGRAM()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    nirvana/vh3/src
    alice/quality/collect_marks_graph/graph
    library/python/nirvana
)

RESOURCE(
    ../config/navi.json navi.json
    ../config/searchapp.json searchapp.json
    ../config/speakers.json speakers.json
    ../config/tv.json tv.json
)

PY_SRCS(MAIN gen.py)

END()
