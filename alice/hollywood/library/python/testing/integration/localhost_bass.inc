# This should be INCLUDEd to ya.make of every it/generator tests target that uses localhost BASS

DEPENDS(
    alice/bass/bin
    alice/bass/scripts/run
)

DATA(
    arcadia/alice/bass/configs/localhost_config.json
)
