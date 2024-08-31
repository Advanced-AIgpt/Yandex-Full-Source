PY2_PROGRAM(gc_skill)

OWNER(g:alice)

PEERDIR(
    alice/vins/core
    alice/vins/sdk
    alice/vins/apps/pa_skills
)

PY_SRCS(
    alice/vins/apps/pa_skills/gc_skill/__main__.py
)

END()
