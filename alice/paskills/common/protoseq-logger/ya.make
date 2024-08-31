JAVA_PROGRAM(protoseq-logger)

OWNER(
    g:paskills
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
JDK_VERSION(17)

PEERDIR(
    alice/library/go/setrace/protos

    contrib/java/org/springframework/spring-webmvc
    contrib/java/jakarta/servlet/jakarta.servlet-api
    contrib/java/org/apache/logging/log4j/log4j-core
    contrib/java/com/google/code/findbugs/jsr305
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.logging.protoseq SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    protoseq-logger
    test-results
    src/test/test-results
)

END()

# RECURSE_FOR_TESTS(src/test)

