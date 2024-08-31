LIBRARY()

OWNER(g:bass)

PEERDIR(
    library/cpp/http/misc
    library/cpp/scheme
)

SRCS(
    error.cpp
    generic_error.cpp
)

GENERATE_ENUM_SERIALIZATION(error.h)

END()
