LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    locale.cpp
)

PEERDIR(
    kernel/relev_locale
    kernel/relev_locale/protos
    search/begemot/rules/internal/locale/proto
    library/cpp/langmask
    library/cpp/langs
)

END()
