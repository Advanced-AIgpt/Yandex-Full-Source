JAVA_PROGRAM(afisha-scenario)

OWNER(g:paskills)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/kronstadt/core
    alice/paskills/common/apphost-http-request
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.scenarios.afisha SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

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

RECURSE_FOR_TESTS(src/test)