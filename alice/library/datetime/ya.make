LIBRARY()

OWNER(g:megamind)

PEERDIR(
    library/cpp/scheme
    library/cpp/timezone_conversion
)

SRCS(
    datetime.cpp
)

END()

RECURSE_FOR_TESTS(ut)
