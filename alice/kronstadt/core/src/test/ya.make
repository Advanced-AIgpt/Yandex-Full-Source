OWNER(
    g:paskills
)

JUNIT5(kronstadt-core-ut)


SIZE(SMALL)

WITH_KOTLIN()
JDK_VERSION(17)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.core SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)


PEERDIR(
    alice/kronstadt/core

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
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/kronstadt/core/kronstadt-core.jar
)

JAVAC_FLAGS(
    -parameters
)

END()
