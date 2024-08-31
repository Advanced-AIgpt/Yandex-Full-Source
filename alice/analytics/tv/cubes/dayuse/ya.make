OWNER(g:sda)

PY2_LIBRARY()

PY_SRCS(
    main.py
    TOP_LEVEL    dayuse_reducer.py
    dayuse_utils.py
    TOP_LEVEL    constants.py
    hdmi_fields.py
    hdmi_info.py
    daily_fields.py
    ads_fields.py
    screens_info.py
    logins_info.py
    apps_timespent.py
    carousels_info.py
)

PEERDIR(
    statbox/nile
    alice/analytics/utils
    alice/analytics/utils/relative_libs/relative_lib_common
    alice/analytics/utils/relative_libs/relative_lib_yt
    alice/analytics/utils/nirvana
    alice/analytics/utils/yt
    alice/analytics/utils/tv
    statbox/nile
    statbox/qb2
)

END()