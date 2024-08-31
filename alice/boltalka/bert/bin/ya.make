PROGRAM()

SRCS(
	main.cpp
)

IF (USE_GPU)
    CFLAGS(-DUSE_GPU)
ENDIF()

PEERDIR(
	alice/boltalka/bert/lib
)

END()