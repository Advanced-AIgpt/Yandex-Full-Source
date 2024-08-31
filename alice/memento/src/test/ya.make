OWNER(
    g:paskills
)

JUNIT5(memento-test)

SIZE(MEDIUM)
JDK_VERSION(17)
WITH_KOTLIN()

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

ENV(YDB_DEFAULT_LOG_LEVEL="CRIT")
ENV(YDB_ADDITIONAL_LOG_CONFIGS="GRPC_SERVER:DEBUG,TICKET_PARSER:WARN")
ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)


TEST_CWD(alice/memento)

PEERDIR(
    apphost/tools/java_grpc_client
    alice/memento
    contrib/java/org/mockito/mockito-core
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
    contrib/java/org/projectlombok/lombok
)

KOTLINC_FLAGS(
    -Xjvm-default=enable
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/memento/memento.jar
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
)

END()
