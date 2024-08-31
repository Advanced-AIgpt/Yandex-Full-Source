PROGRAM(bass_release_bot)

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/fetcher
    alice/bass/libs/logging_v2
    alice/bass/libs/ydb_kv

    library/cpp/getopt
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/scheme
    library/cpp/sighandler
    library/cpp/threading/future
)

SRCS(
    main.cpp
)

END()
