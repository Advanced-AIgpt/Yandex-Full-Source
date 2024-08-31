PY3_PROGRAM()

OWNER(mkamalova)

PEERDIR(
    contrib/python/click
    library/python/resource
    yt/python/client

    search/scraper_over_yt/mapper/lib/proto
)

RESOURCE(
    alice/acceptance/modules/request_generator/configs/flags/production.json flags_production.json
)

PY_SRCS(
    __main__.py
)

END()


