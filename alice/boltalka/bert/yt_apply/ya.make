PROGRAM()

SRCS(
	main.cpp
)

IF (USE_GPU)
    CFLAGS(-DUSE_GPU)
ENDIF()

PEERDIR(
    alice/boltalka/generative/service/server/handlers
    dict/mt/libs/nn/ynmt/extra
    mapreduce/yt/client
    mapreduce/yt/interface
    library/cpp/getopt/small
)

END()
