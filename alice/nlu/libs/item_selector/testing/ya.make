LIBRARY()

OWNER(volobuev)

PEERDIR(
    alice/nlu/libs/item_selector/interface
    library/cpp/testing/gmock_in_unittest
    util
)

SRCS(
    mock.cpp
)

END()
