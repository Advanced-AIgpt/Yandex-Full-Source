PROGRAM()

OWNER(makatunkin)

PEERDIR(
    contrib/libs/uuid
    kernel/factor_storage
    library/cpp/getopt
    library/cpp/json
    library/cpp/string_utils/base64
    mapreduce/yt/client
    mapreduce/yt/interface
)

SRCS(
    main.cpp
)

END()
