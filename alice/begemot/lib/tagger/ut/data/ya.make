UNION()

OWNER(
    the0
    g:alice
)

RUN_PROGRAM(
    alice/begemot/tools/dump_granet_config ru ${ARCADIA_ROOT}/alice/nlu/data/ru/granet/main.grnt
    IN ${ARCADIA_ROOT}/alice/nlu/data/ru/granet/main.grnt
    STDOUT granet_config.ru.pb.txt
)

END()

