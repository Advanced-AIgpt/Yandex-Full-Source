UNITTEST()

SIZE(MEDIUM)

TAG(ya:external)

OWNER(
    g:alice_downloaders
)

NEED_CHECK()

SRCS(
    ut.cpp
)

DATA(
    sbr://2479875247  # address.opus
)

DEPENDS(
    alice/acceptance/modules/request_generator/scrapper/bin
    alice/uniproxy/mapper/test_bin/bin
)

PEERDIR(
    alice/uniproxy/mapper/fetcher/lib
    alice/uniproxy/mapper/library/logging
    library/cpp/testing/unittest
    library/cpp/yson/node
    search/scraper_over_yt/mapper/lib
)

REQUIREMENTS(network:full)

END()
