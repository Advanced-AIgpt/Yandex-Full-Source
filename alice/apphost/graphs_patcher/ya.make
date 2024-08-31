PY3_PROGRAM(graph_generator)

OWNER(akhruslan)

PEERDIR(
    alice/apphost/library
)

PY_SRCS(
    __main__.py
)

DATA(
    arcadia/apphost/conf/verticals/ALICE
)

END()
