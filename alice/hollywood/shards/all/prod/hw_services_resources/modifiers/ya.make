UNION()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/shards/all/prod/hw_services_resources/modifiers/colored_speaker
    alice/hollywood/shards/all/prod/hw_services_resources/modifiers/conjugator
    alice/hollywood/shards/all/prod/hw_services_resources/modifiers/voice_doodle
)

END()

RECURSE(
    colored_speaker
    conjugator
    voice_doodle
)
