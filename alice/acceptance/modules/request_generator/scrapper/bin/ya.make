PY3_PROGRAM(downloader_inner)

OWNER(mkamalova)

PEERDIR(
    contrib/python/click
    library/python/cyson

    alice/acceptance/modules/request_generator/lib
    alice/acceptance/modules/request_generator/scrapper/lib
)

PY_SRCS(
    __main__.py
)

END()
