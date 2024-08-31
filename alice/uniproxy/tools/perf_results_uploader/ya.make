PY3_PROGRAM(perf_results_uploader)

OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/perf_tester
    yt/python/client
    voicetech/common/lib
)

PY_SRCS(
    MAIN
    main.py
)

END()
