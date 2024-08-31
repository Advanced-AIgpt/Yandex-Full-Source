LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

SRCS(
    music_sdk_uri_builder.cpp
)

GENERATE_ENUM_SERIALIZATION(music_sdk_uri_builder.h)

END()
