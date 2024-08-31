PY3_PROGRAM()

OWNER(
    g:alice_downloaders
)

PEERDIR(
    contrib/python/click
    library/python/cyson
    library/python/resource
    yt/python/client
    alice/acceptance/modules/request_generator/lib
    alice/acceptance/modules/request_generator/scrapper/lib
    alice/uniproxy/mapper/test_bin/lib
)

PY_SRCS(
    __main__.py
)

RESOURCE(
    alice/uniproxy/mapper/test_bin/data/data.json data.json
)

END()
