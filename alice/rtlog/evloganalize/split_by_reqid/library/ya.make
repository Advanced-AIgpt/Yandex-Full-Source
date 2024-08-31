LIBRARY()

OWNER(sparkle)

PEERDIR(
    alice/rtlog/protos
    library/cpp/eventlog/dumper
    library/cpp/logger/global
)

SRCS(
    splitter.cpp
)

END()
