OWNER(
    alipov
    g:alice_boltalka
)
# These tool depends on contrib/libs/tf which builds under linux only (at least for now)
IF(OS_LINUX)
    PROGRAM()

    SRCS(
        main.cpp
    )

    PEERDIR(
        alice/boltalka/libs/text_utils
        mapreduce/yt/client
        mapreduce/yt/interface
        dict/mt/libs/punctuation
        dict/mt/libs/punctuation/enc_tf
        dict/mt/libs/filters
        library/cpp/getopt/small
        util
    )

    ALLOCATOR(LF)

    END()
ENDIF()
