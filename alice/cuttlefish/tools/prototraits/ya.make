PROGRAM()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/libs/protobuf
    contrib/libs/protoc
    alice/cuttlefish/tools/prototraits/protos
)
SRCS(
    field_methods.cpp
    field_methods.h
    generator.cpp
    generator.h
    main.cpp
    prototraits.h
    utils.cpp
    utils.h
)

INDUCED_DEPS(h+cpp
    ${ARCADIA_ROOT}/alice/cuttlefish/tools/prototraits/prototraits.h
)
END()
