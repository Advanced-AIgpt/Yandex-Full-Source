PROGRAM()

OWNER(
    deemonasd
    g:bass
)

PEERDIR(
    library/cpp/getopt
    library/cpp/scheme
    library/cpp/string_utils/quote
    mapreduce/yt/client
    mapreduce/yt/interface
)

SRCS(
    main.cpp
    constants.cpp
)

END()
