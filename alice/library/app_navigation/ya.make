LIBRARY()

OWNER(g:alice)

CFLAGS(-DPIRE_NO_CONFIG)

SRCS(
    bno_apps_trie.cpp
    bno_apps.cpp
    fixlist.cpp
    navigation.cpp
)

PEERDIR(
    alice/hollywood/library/request
    alice/library/client
    alice/library/logger
    alice/library/url_builder
    library/cpp/containers/comptrie
    library/cpp/resource
    library/cpp/scheme 
    library/cpp/threading/hot_swap
)

END()
