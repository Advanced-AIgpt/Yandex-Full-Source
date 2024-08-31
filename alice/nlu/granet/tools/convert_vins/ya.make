PROGRAM()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    convert_music.cpp
    main.cpp
)

PEERDIR(
    alice/nlu/granet/lib
    alice/nlu/granet/tools/common
    library/cpp/dbg_output
    library/cpp/getopt/small
)

END()
