LIBRARY()

OWNER(g:alice)

SRCS(
    file_system_loader.cpp
    fst_base_rule.cpp
)

PEERDIR(
    alice/nlu/proto/entities
    alice/nlu/libs/fst
    contrib/libs/protobuf
    library/cpp/langs
    search/begemot/core
    search/begemot/rules/text/proto
)

END()



