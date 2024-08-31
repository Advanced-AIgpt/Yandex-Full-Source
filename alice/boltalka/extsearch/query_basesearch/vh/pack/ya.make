PY3_PROGRAM()
OWNER(g:alice_boltalka)
PY_MAIN(alice.boltalka.extsearch.query_basesearch.vh.pack:packed_op_main)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/boltalka/extsearch/query_basesearch/vh
)

END()
