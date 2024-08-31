JAVA_LIBRARY(fstnormalizer)

JDK_VERSION(11)

OWNER(
    g:alice_quality
    g:paskills
)

PEERDIR(
    contrib/java/com/google/code/findbugs/jsr305/3.0.2
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/request_normalizer/java/src/c
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.nlu.libs.fstnormalizer src/java/main/**/*)

NO_LINT()

END()

RECURSE_FOR_TESTS(
    src/java/test
)