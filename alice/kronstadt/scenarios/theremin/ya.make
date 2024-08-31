JAVA_LIBRARY(theremin-scenario)

OWNER(g:paskills)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/springframework/boot/spring-boot-starter-data-jdbc
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/org/postgresql/postgresql

    alice/nlu/libs/request_normalizer/java
    alice/paskills/common/pgconverter

    alice/kronstadt/core
    alice/paskills/dialogovo/src/main/proto
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskill.dialogovo.scenarios.theremin SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
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
