OWNER(ferume-oh3)

PY2_LIBRARY()

SRCDIR(alice/analytics/operations/dialog/sessions)

PY_SRCS(
    intent_scenario_mapping.py
    usage_fields.py
    make_dialog_sessions.py
    make_video_squeeze.py
    replica_constructor.py
)

PEERDIR(
    alice/analytics/operations/dialog/sessions/sessions_runner/utils_nirvana_workaround
    alice/analytics/utils/yt/relative_lib
    alice/library/client/protos
    alice/megamind/protos/analytics
    alice/wonderlogs/sdk/python
    contrib/python/protobuf
    library/python/resource
    statbox/nile
    statbox/qb2
    yql/library/python
)

END()
