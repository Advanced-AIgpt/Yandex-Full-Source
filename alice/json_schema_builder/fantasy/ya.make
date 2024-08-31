PROGRAM()

OWNER(g:alice)

PEERDIR(
    alice/json_schema_builder/runtime
)

SRCS(
    div2.cpp
    main.cpp
)

GENERATE_ENUM_SERIALIZATION(
    enums.h
)

END()
