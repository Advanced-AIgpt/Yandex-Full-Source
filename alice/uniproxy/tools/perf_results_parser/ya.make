PY3_PROGRAM(perf_results_parser)

OWNER(abezhin)

PEERDIR(
    contrib/python/prettytable
    voicetech/common/lib
)

PY_SRCS(
    MAIN
    main.py
)

END()
