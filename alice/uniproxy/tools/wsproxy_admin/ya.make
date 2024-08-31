PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    MAIN
    main.py
    __init__.py
    argparser.py
)

PEERDIR(
    contrib/python/aiohttp
)

END()

RECURSE_FOR_TESTS(ut)
