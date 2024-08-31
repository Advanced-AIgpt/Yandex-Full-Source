JUNIT5()

OWNER(g:paskills)

JDK_VERSION(17)
WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

PEERDIR(
    alice/library/java/protobuf_utils
    alice/library/java/protobuf_utils/src/test/proto
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
    contrib/java/org/junit/jupiter/junit-jupiter

    contrib/java/org/openjdk/jmh/jmh-core
    contrib/java/org/openjdk/jmh/jmh-generator-annprocess

)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.library.protobufutils SRCDIR  java **/*)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/library/java/protobuf_utils/alice-library-protobuf-utils.jar
)

ANNOTATION_PROCESSOR(org.openjdk.jmh.generators.BenchmarkProcessor)

LINT(extended)

END()


