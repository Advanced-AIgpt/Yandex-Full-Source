PY3_PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    main.py
)

PY_MAIN(
    alice.boltalka.extsearch.query_basesearch.main
)

PEERDIR(
    alice/boltalka/extsearch/query_basesearch/lib/grequests
)

END()

RECURSE(
    lib
)
