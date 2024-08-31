OWNER(
    g:paskills
)

JUNIT5(theremin-scenario-ut)

SIZE(MEDIUM)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskill.dialogovo.scenarios.theremin SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/kronstadt/core-test
    alice/kronstadt/server
    alice/kronstadt/scenarios/theremin
    # alice/paskills/dialogovo/src/test/proto
    contrib/java/org/mockito/mockito-core
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/junit/junit
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
#FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/kronstadt/scenarios/theremin/theremin-scenario.jar
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    theremin-scenario-ut
    test-results
)

END()
