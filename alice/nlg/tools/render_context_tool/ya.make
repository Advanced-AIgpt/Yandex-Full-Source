PROGRAM()

OWNER(g:alice)

PEERDIR(
    alice/hollywood/library/registry
    alice/hollywood/library/framework

    alice/hollywood/library/base_scenario
    alice/hollywood/library/scenarios/alarm
    alice/hollywood/library/scenarios/fast_command
    alice/hollywood/library/scenarios/music
    alice/hollywood/library/scenarios/news
    alice/hollywood/library/scenarios/search
    alice/hollywood/library/scenarios/weather

    alice/nlg/tools/render_context_tool/proto

    library/cpp/getoptpb
)


SRCS(
    main.cpp
)

END()

RECURSE(
    data
)
