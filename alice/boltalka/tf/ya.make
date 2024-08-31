# Example C++ applicator of dialog models
PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

PEERDIR(
    contrib/libs/tf
    library/cpp/getopt
)

SRCS(
    applicator.cc
)
END()
