LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/config
    alice/bass/libs/forms_db/delegate
    alice/bass/libs/logging_v2
    alice/bass/libs/scheduler
    alice/bass/libs/source_request
    library/cpp/scheme
)

SRCS(
    external_skill_db.cpp
)

END()

