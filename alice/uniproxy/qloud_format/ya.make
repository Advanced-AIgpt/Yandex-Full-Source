PROGRAM(qloud_format)
OWNER(g:voicetech-infra)

PEERDIR(library/cpp/json)
SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(ut)