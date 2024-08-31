LIBRARY()

OWNER(g:alice_boltalka g:zeliboba)

PEERDIR(
    alice/boltalka/generative/inference/core
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/service/server/config

    kernel/bert
    alice/library/logger
    alice/boltalka/libs/text_utils
    alice/joker/library/s3
    alice/joker/library/status
    dict/mt/libs/nn/ynmt/extra
    dict/mt/libs/nn/ynmt_backend
    library/cpp/cache
    library/cpp/string_utils/base64
    mapreduce/yt/client
    mapreduce/yt/interface
)

SRCS(
    bert_factor_request_handler.cpp
    proto_request_handler.cpp
    ptune_storage.cpp
)

END()
