OWNER(g:alice_analytics)

PY23_LIBRARY()

PY_SRCS(
    generic_scenario_to_human_readable.py
    visualize_request.py
    visualize_directive.py
    visualize_state.py

    utils.py

    visualize_alarms_timers.py
    visualize_answer_standard.py
    visualize_geo.py
    visualize_iot.py
    visualize_multiroom.py
    visualize_music.py
    visualize_open_uri.py
    visualize_video.py
)

NO_CHECK_IMPORTS()

RESOURCE_FILES(
    PREFIX alice/analytics/operations/priemka/alice_parser/visualize/

    i18n/en/LC_MESSAGES/generic_scenario_to_human_readable.mo
    i18n/en/LC_MESSAGES/visualize_alarms_timers.mo
    i18n/en/LC_MESSAGES/visualize_directive.mo
    i18n/en/LC_MESSAGES/visualize_geo.mo
    i18n/en/LC_MESSAGES/visualize_iot.mo
    i18n/en/LC_MESSAGES/visualize_multiroom.mo
    i18n/en/LC_MESSAGES/visualize_music.mo
    i18n/en/LC_MESSAGES/visualize_open_uri.mo
    i18n/en/LC_MESSAGES/visualize_state.mo
    i18n/en/LC_MESSAGES/visualize_video.mo

    i18n/ar/LC_MESSAGES/generic_scenario_to_human_readable.mo
    i18n/ar/LC_MESSAGES/visualize_alarms_timers.mo
    i18n/ar/LC_MESSAGES/visualize_directive.mo
    i18n/ar/LC_MESSAGES/visualize_geo.mo
    i18n/ar/LC_MESSAGES/visualize_iot.mo
    i18n/ar/LC_MESSAGES/visualize_multiroom.mo
    i18n/ar/LC_MESSAGES/visualize_music.mo
    i18n/ar/LC_MESSAGES/visualize_open_uri.mo
    i18n/ar/LC_MESSAGES/visualize_state.mo
    i18n/ar/LC_MESSAGES/visualize_video.mo
)


END()

RECURSE_ROOT_RELATIVE(
    alice/analytics/operations/priemka/alice_parser/tests
)
