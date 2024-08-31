LIBRARY()

SRCS(
    local_executor_wrapper.cpp
)

PEERDIR(
    alice/nlu/granet/lib
    alice/nlu/granet/lib/parser

    library/cpp/threading/local_executor
)

END()

