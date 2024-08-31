DLL(fstnormalizer_java)

#EXPORTS_SCRIPT(fstnormalizer_java.exports)

OWNER(
    g:alice_quality
    g:paskills
)

SRCS(
    ru_yandex_alice_nlu_libs_fstnormalizer_FstNormalizer.cpp
)

IF (OS_LINUX)
    LDFLAGS(-Wl,-z,noexecstack)
ENDIF()

PEERDIR(
    contrib/libs/jdk
    alice/nlu/libs/request_normalizer
)

END()