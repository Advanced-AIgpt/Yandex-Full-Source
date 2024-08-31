UNION()

OWNER(
    akhruslan
    g:hollywood
)

FILES(.svninfo)

IF (SCENARIO_ALICE_SHOW OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/alice_show)
ENDIF() 

IF (SCENARIO_BLUEPRINTS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/blueprints)
ENDIF() 

IF (SCENARIO_GENERAL_CONVERSATION OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/shards/all/prod/fast_data/general_conversation
        alice/hollywood/shards/all/prod/fast_data/general_conversation_proactivity
    )
ENDIF()

IF (SCENARIO_HARDCODED_RESPONSE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/hardcoded_response)
ENDIF()

IF (SCENARIO_HOW_TO_SPELL OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/how_to_spell)
ENDIF()

IF (SCENARIO_MARKET OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/market)
ENDIF()

IF (SCENARIO_MUSIC OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/music)
ENDIF()

IF (SCENARIO_NEWS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/news)
ENDIF()

IF (SCENARIO_NOTIFICATIONS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/notifications)
ENDIF()

IF (SCENARIO_RANDOM_NUMBER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/random_number)
ENDIF()

IF (SCENARIO_SSSSS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/fast_data/sssss)
ENDIF()

END()

IF (SCENARIO_ALICE_SHOW OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(alice_show)
ENDIF()

IF (SCENARIO_GENERAL_CONVERSATION OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(
        general_conversation
        general_conversation_proactivity
    )
ENDIF()

IF (SCENARIO_HARDCODED_RESPONSE OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(hardcoded_response)
ENDIF()

IF (SCENARIO_HOW_TO_SPELL OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(how_to_spell)
ENDIF()

IF (SCENARIO_MARKET OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(market)
ENDIF()

IF (SCENARIO_MUSIC OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(music)
ENDIF()

IF (SCENARIO_NEWS OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(news)
ENDIF()

IF (SCENARIO_NOTIFICATIONS OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(notifications)
ENDIF()

IF (SCENARIO_RANDOM_NUMBER OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(random_number)
ENDIF()

IF (SCENARIO_SSSSS OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(sssss)
ENDIF()
