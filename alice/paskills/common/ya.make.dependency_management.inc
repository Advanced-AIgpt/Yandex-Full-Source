INCLUDE(${ARCADIA_ROOT}/contrib/java/org/junit/junit-bom/5.8.2/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/springframework/boot/spring-boot-dependencies/2.6.6/ya.dependency_management.inc)
# see https://github.com/grpc/grpc-java/issues/8981
INCLUDE(${ARCADIA_ROOT}/contrib/java/io/netty/netty-bom/4.1.74.Final/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/apache/logging/log4j/log4j-bom/2.17.2/ya.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

EXCLUDE(
    contrib/java/com/google/protobuf/protobuf-javalite
)

DEPENDENCY_MANAGEMENT(

    contrib/java/org/projectlombok/lombok/1.18.22
    contrib/java/com/google/code/findbugs/jsr305/3.0.2
    contrib/java/com/google/guava/guava/30.1.1-jre
    contrib/java/org/postgresql/postgresql/42.3.3
    contrib/java/commons-validator/commons-validator/1.7
    contrib/java/commons-lang/commons-lang/2.6
    contrib/java/com/beust/jcommander/1.81
    contrib/java/org/slf4j/slf4j-api/1.7.36

    contrib/java/net/bytebuddy/byte-buddy/1.12.7
    contrib/java/net/bytebuddy/byte-buddy-agent/1.12.7

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor/3.4.2

    # testing
    contrib/java/junit/junit/4.13.2
    contrib/java/com/squareup/okhttp3/mockwebserver/3.12.0
    contrib/java/commons-io/commons-io/2.9.0
    contrib/java/ru/yandex/qatools/embed/postgresql-embedded/2.10
    contrib/java/de/flapdoodle/embed/de.flapdoodle.embed.process/2.0.5 # /2.1.2
    contrib/java/org/skyscreamer/jsonassert/1.5.0
    contrib/java/org/mockito/mockito-core/4.3.1
    contrib/java/org/mockito/kotlin/mockito-kotlin/4.0.0

    contrib/java/org/openjdk/jmh/jmh-core/1.34
    contrib/java/org/openjdk/jmh/jmh-generator-annprocess/1.34
)
