LIBRARY()

OWNER(sparkle)

PEERDIR(
    alice/joker/library/log
    alice/rtlog/evloganalize/split_by_reqid/library
    alice/tests/difftest/shooter/library/ports
    alice/tests/difftest/shooter/library/yav
    contrib/libs/re2
    library/cpp/neh
    library/cpp/scheme
    library/cpp/uri
    apphost/lib/client
    apphost/lib/grpc/client
    apphost/lib/grpc/json
    apphost/lib/grpc/protos
)

SRCS(
    apps/decorated_app.cpp
    apps/impl/bass_app.cpp
    apps/impl/hollywood_app.cpp
    apps/impl/joker_app.cpp
    apps/impl/megamind_app.cpp
    apps/impl/redis_app.cpp
    apps/impl/vins_app.cpp
    config.cpp
    config.sc
    context.cpp
    engine.cpp
    factory/hollywood_bass_factory.cpp
    factory/hollywood_factory.cpp
    factory/megamind_factory.cpp
    requester/hollywood_bass_requester.cpp
    requester/hollywood_requester.cpp
    requester/megamind_requester.cpp
    run_settings.cpp
    util.cpp
)

END()
