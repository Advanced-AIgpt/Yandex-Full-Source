LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/request
    
    alice/library/client
    alice/library/data_sync
    alice/library/logger
)

SRCS(
    personal_data.cpp
)

END()

RECURSE_FOR_TESTS(ut)
