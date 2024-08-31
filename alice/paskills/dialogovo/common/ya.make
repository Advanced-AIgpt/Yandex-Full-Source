JAVA_LIBRARY(dialogovo-common)

OWNER(
    g:paskills
)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/apache/httpcomponents/httpclient

    contrib/java/org/projectlombok/lombok

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    alice/paskills/common/instrumented-executor
    alice/paskills/common/rest-template-factory
    alice/paskills/common/protoseq-logger

    alice/kronstadt/core
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskill.dialogovo SRCDIR src/main/java **/*)
#JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    -Xopt-in=kotlin.RequiresOptIn
)

JAVAC_FLAGS(
    -parameters
)

END()
