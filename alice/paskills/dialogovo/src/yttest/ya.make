OWNER(
    g:paskills
)

JUNIT5(dialogovo-yttest)

SIZE(MEDIUM)

JDK_VERSION(17)

JAVA_SRCS(SRCDIR ${ARCADIA_ROOT}/alice/paskills/dialogovo/src/yttest/java **/*)

DATA(
    arcadia/alice/paskills/dialogovo/config
)

INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

TEST_CWD(alice/paskills/dialogovo)

PEERDIR(
    alice/paskills/dialogovo
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
    iceberg/inside-yt
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/junit/junit
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

JVM_ARGS(
    --add-opens=java.base/java.lang=ALL-UNNAMED
)
END()
