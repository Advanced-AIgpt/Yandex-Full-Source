LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

SRCS(
    tokenizer.h
)

PEERDIR(
    dict/mt/libs/filters/segmenter
    dict/mt/libs/libmt
    kernel/lemmer/core
    library/cpp/langs
)

END()
