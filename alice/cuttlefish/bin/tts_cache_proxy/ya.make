PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/tts/cache/proxy
    alice/cuttlefish/library/service_runner
)

SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(
    tests
)
