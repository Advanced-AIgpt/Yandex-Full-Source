JAVA_LIBRARY(photoframe-scenario)

OWNER(g:paskills)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/kronstadt/core
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskill.dialogovo.scenarios.photoframe SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -java-parameters
)

JAVAC_FLAGS(
    -parameters
)

END()

RECURSE_FOR_TESTS(
    it2
)
