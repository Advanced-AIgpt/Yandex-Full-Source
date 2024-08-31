OWNER(g:voicetech-infra)

UNION()

FILES(experiments_rtc_production.json)
FILES(experiments_ycloud.json)
FILES(vins_experiments.json)

END()

RECURSE_ROOT_RELATIVE(alice/uniproxy/library/event_patcher)
RECURSE_FOR_TESTS(ut)
