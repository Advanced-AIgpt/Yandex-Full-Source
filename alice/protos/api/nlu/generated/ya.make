PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    vl-trifonov
    g:alice_quality
)

IF (GEN_PROTO)
    RUN_PROGRAM(
        alice/begemot/tools/gen_feature_enum
        alice/nlu/data/ru/config/features.pb.txt
        features.proto
        ENluFeature
        --proto-package=NAlice.NNluFeatures
        --go-package=a.yandex-team.ru/alice/protos/api/nlu/generated
        --java-package=ru.yandex.alice.protos.api.nlu.generated
        IN alice/nlu/data/ru/config/features.pb.txt
        OUT_NOAUTO features.proto
    )
ENDIF()

SRCS(
    features.proto
)

END()
