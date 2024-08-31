JAVA_PROGRAM(billing_yt_snapshots_cleaner)

JDK_VERSION(11)

OWNER(
    g:paskills
)

WITH_KOTLIN()


INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/projectlombok/lombok
    contrib/java/com/beust/jcommander
    contrib/java/org/apache/logging/log4j/log4j-core
    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    iceberg/inside-yt
)

JAVA_SRCS(PACKAGE_PREFIX ru.alice.paskills.billing.yt.cleaner SRCDIR src/main/kotlin **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()
