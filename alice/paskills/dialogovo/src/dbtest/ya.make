OWNER(
    g:paskills
)

JUNIT5(dialogovo-dbtest)

WITH_KOTLIN()
JDK_VERSION(17)

SIZE(MEDIUM)

ENV(YDB_DEFAULT_LOG_LEVEL="CRIT")
ENV(YDB_ADDITIONAL_LOG_CONFIGS="GRPC_SERVER:DEBUG,TICKET_PARSER:WARN")

JAVA_SRCS(SRCDIR ${ARCADIA_ROOT}/alice/paskills/dialogovo/src/dbtest/java **/*)
JAVA_SRCS(SRCDIR ${ARCADIA_ROOT}/alice/paskills/dialogovo/src/dbtest/resources **/*)

DATA(
    arcadia/alice/paskills/dialogovo/config
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

TEST_CWD(alice/paskills/dialogovo)

PEERDIR(
    alice/paskills/dialogovo
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
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/paskills/dialogovo/dialogovo.jar
)

JAVAC_FLAGS(
    -parameters
)

END()
