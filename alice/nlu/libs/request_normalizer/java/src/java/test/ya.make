JUNIT5()

JDK_VERSION(11)

OWNER(
    g:alice_quality
    g:paskills
)

INCLUDE(${ARCADIA_ROOT}/contrib/java/org/junit/junit-bom/5.5.2/junit-bom.bom.inc)
PEERDIR(
    alice/nlu/libs/request_normalizer/java
    alice/nlu/libs/request_normalizer/java/src/c
    contrib/java/org/junit/jupiter/junit-jupiter
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.nlu.libs.fstnormalizer **/*)

NO_LINT()

END()
