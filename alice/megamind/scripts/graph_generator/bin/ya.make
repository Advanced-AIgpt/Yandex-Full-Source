PY3_PROGRAM(graph_generator)

OWNER(akhruslan)

PEERDIR(
    alice/megamind/scripts/graph_generator/library
)

PY_SRCS(
    __main__.py
)

DATA(
    arcadia/alice/megamind/configs
    arcadia/apphost/conf/verticals/ALICE
)

RESOURCE(- ARCADIA_ROOT=${ARCADIA_ROOT})

END()
