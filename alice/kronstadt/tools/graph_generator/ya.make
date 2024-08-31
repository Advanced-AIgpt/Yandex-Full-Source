OWNER(g:paskills)

JAVA_PROGRAM(kronstadt-graph-generator)


WITH_JDK()
WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.generator SRCDIR src/main/kotlin **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

DEPENDENCY_MANAGEMENT(
    contrib/java/info/picocli/picocli/4.6.3
)

PEERDIR(
    apphost/lib/proto_config/types_nora
    alice/megamind/library/config/scenario_protos

    contrib/java/info/picocli/picocli
    alice/kronstadt/tools/graph_generator/src/main/proto
    alice/kronstadt/shard_runner
)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    #FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -java-parameters
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(
    alice/megamind/library/classifiers/formulas/protos
)

END()

RECURSE(
    src/main/proto
)
