LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    processor.cpp
)

PEERDIR(
    alice/wonderlogs/library/parsers
    alice/wonderlogs/rt/library/common
    alice/wonderlogs/rt/processors/uniproxy_resharder/protos
    alice/wonderlogs/rt/protos

    ads/bsyeti/big_rt/lib/utility/logging
    ads/bsyeti/big_rt/lib/processing/resharder/lib
    ads/bsyeti/big_rt/lib/processing/resharder/rows_processor

    library/cpp/framing
    library/cpp/logger/global
    library/cpp/safe_stats

    logfeller/lib/chunk_splitter
    logfeller/lib/log_parser
)

END()
