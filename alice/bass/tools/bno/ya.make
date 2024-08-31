PROGRAM()

OWNER(imakunin)

PEERDIR(
    alice/library/app_navigation
    library/cpp/getopt
    library/cpp/containers/comptrie
    library/cpp/uri
    library/cpp/cgiparam
)

SRCS(
    main.cpp
)

END()

RECURSE(
    loader
)
