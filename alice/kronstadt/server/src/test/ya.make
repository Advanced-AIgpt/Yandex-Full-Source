OWNER(
    g:paskills
)

JUNIT5(kronstadr-server-ut)

SIZE(MEDIUM)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.server SRCDIR java **/*)


INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/kronstadt/server
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
    -Xfriend-paths=alice/library/java/kronstadt/server/kronstadr-server.jar
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    kronstadr-server-ut
    test-results
)

END()
