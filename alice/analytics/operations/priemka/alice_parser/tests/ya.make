OWNER(g:alice_analytics)

PY2TEST()

TEST_SRCS(
    utils.py
    test_general.py
    test_basket_fields.py
    test_prepare_downloader_result.py
    test_utils.py
    test_visualize.py
    test_make_session.py
    test_hashable.py
    test_translations.py
)

ENV(LANGUAGE=ru)

DATA(
    sbr://2579252137=tests_data/01_input_data
    sbr://1941429950=tests_data/02_input_data
    sbr://1988127021=test_general  # get_screenshots_data
    sbr://2773670732=test_general  # 03_general_actions.in.json
    sbr://2049562861=test_hashable  # 02_hashable_video_play.json
    sbr://2040897316=test_hashable  # 01_hashable_tv_stream.json
    sbr://2045036913=test_hashable  # 01_hashable_weather.json
    sbr://2049482167=test_hashable  # hashable_weather_pp.json
    sbr://2401519850=test_make_session  # 03_all_ue2e_baskets_intents_directives.in.json
    sbr://2875170319=test_visualize # iot_new_analytics_info_test.in.json
)

# TODO: check needed peerdirs
PEERDIR(
    contrib/python/pytest
    statbox/nile
    statbox/nile_debug
    statbox/qb2
    yql/library/python

    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/operations/priemka/alice_parser/lib
    alice/analytics/operations/priemka/alice_parser/visualize
    alice/analytics/utils/testing_utils
)

END()

RECURSE_FOR_TESTS(
    tests_en
    tests_ar
)
