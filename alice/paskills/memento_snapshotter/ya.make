JAVA_PROGRAM(memento_snapshotter)

OWNER(
    g:paskills
)

WITH_JDK()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    alice/memento/proto
    alice/memento/scanner
    iceberg/inside-yt
    yt/java/ytclient
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
