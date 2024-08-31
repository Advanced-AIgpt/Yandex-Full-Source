PROGRAM()

OWNER(g:voicetech-infra)

NEED_REVIEW()

PEERDIR(
    contrib/libs/protobuf
    contrib/libs/protoc
    contrib/libs/grpc
    alice/gproxy/library/protos/annotations
    alice/protos/extensions
)

SRCS(
    main.cpp
    generator.cpp
    printer.cpp
    service.cpp
    entity.cpp
)

END()
