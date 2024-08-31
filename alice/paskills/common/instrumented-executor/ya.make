OWNER(g:paskills)

JAVA_LIBRARY(paskills-instrumented-executor)

JDK_VERSION(11)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

PEERDIR(
    contrib/java/com/google/code/findbugs/jsr305
    contrib/java/com/google/guava/guava

    alice/paskills/common/solomon-utils
    library/java/monlib/metrics
    library/java/annotations
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.executor SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
)

END()

RECURSE_FOR_TESTS(
    src/test
)
