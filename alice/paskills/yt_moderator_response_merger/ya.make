JAVA_PROGRAM(yt_moderator_response_merger)

JDK_VERSION(11)

OWNER(
    g:paskills
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/projectlombok/lombok
    contrib/java/com/beust/jcommander
    contrib/java/org/apache/logging/log4j/log4j-core

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    iceberg/inside-yt
)

JAVA_SRCS(SRCDIR src/main/java **/*)

UBERJAR()

UBERJAR_PATH_EXCLUDE_PREFIX(
    META-INF/*.SF
    META-INF/*.RSA
    META-INF/*.DSA
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()
