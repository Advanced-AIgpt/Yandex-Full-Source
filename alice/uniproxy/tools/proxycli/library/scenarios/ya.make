LIBRARY()

OWNER(
    g:voicetech-infra
)

RESOURCE(
    asr.ru.dialogeneral.yaml            /scenarios/asr.ru.dialogeneral.yaml
    asr.ru.freeform.yaml                /scenarios/asr.ru.freeform.yaml
    asr.ru.general.yaml                 /scenarios/asr.ru.general.yaml
    asr.ru.quasar-general.yaml          /scenarios/asr.ru.quasar-general.yaml
    asr.ru.all.yaml                     /scenarios/asr.ru.all.yaml
    asr.basic.yaml                      /scenarios/asr.basic.yaml

    spotter.basic.yaml                  /scenarios/spotter.basic.yaml
    spotter.ru.dialogeneral.yaml        /scenarios/spotter.ru.dialogeneral.yaml
    spotter.ru.dialogmaps-gpu.yaml      /scenarios/spotter.ru.dialogmaps-gpu.yaml
    spotter.ru.dialogmaps.yaml          /scenarios/spotter.ru.dialogmaps.yaml
    spotter.ru.quasargeneral-gpu.yaml   /scenarios/spotter.ru.quasargeneral-gpu.yaml
    spotter.ru.quasargeneral.yaml       /scenarios/spotter.ru.quasargeneral.yaml

    spotter.all.yaml                    /scenarios/spotter.all.yaml
    spotter.gpu.all.yaml                /scenarios/spotter.gpu.all.yaml

    smarthome.turnoff.yaml              /scenarios/smarthome.turnoff.yaml

    system.fast-events.yaml             /scenarios/system.fast-events.yaml

    logrequeststat.yaml                 /scenarios/logrequeststat.yaml

    tts.fast-events.yaml                /scenarios/tts.fast-events.yaml

    all.yaml                            /scenarios/all.yaml

    debug.yaml                          /scenarios/debug.yaml

    whathaveyoudone.opus                /scenarios/data/whathaveyoudone.opus
    spotter.opus                        /scenarios/data/spotter.opus
    hello.opus                          /scenarios/data/hello.opus
    turnoff.opus                        /scenarios/data/turnoff.opus
)

END()
