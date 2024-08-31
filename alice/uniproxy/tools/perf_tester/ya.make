PY3_PROGRAM(perf_tester)

OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/perf_tester
    contrib/python/tqdm
    yt/python/client
    voicetech/asr/tools/robin/lib
    voicetech/common/lib
)

PY_SRCS(
    MAIN
    main.py
)

NO_CHECK_IMPORTS(
    alice.uniproxy.tools.perf_tester.main
    voicetech.asr.tools.robin.lib.*
)

END()
