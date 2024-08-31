PROGRAM()

OWNER(
    egor-suhanov
)

SRCS(
    main.cpp
    dataset.cpp
    common.cpp
    parser.cpp
    parse_applet.cpp
    debug_applet.cpp
)

PEERDIR(
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/fst
    library/cpp/testing/common
    library/cpp/getopt/small
    library/cpp/getopt
    library/cpp/langs
)

END()
