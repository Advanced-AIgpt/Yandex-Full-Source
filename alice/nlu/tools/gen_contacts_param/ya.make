PROGRAM()

OWNER(
    deeminasd
    g:alice_quality
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/megamind/protos/common
    alice/library/contacts
    library/cpp/string_utils/base64
    library/cpp/protobuf/json
    library/cpp/getopt
)

END()
