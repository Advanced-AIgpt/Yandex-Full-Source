INCLUDE(${ARCADIA_ROOT}/contrib/java/org/junit/junit-bom/5.8.2/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/springframework/boot/spring-boot-dependencies/2.6.6/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/apache/logging/log4j/log4j-bom/2.17.2/ya.dependency_management.inc)
DEPENDENCY_MANAGEMENT(

    contrib/java/com/google/code/findbugs/jsr305/3.0.2
    contrib/java/com/google/guava/guava/30.1.1-jre
    contrib/java/org/postgresql/postgresql/42.3.3
    contrib/java/org/apache/httpcomponents/httpclient/4.5.13
    contrib/java/commons-io/commons-io/2.11.0
    contrib/java/org/json/json/20210307

    contrib/java/org/projectlombok/lombok/1.18.22
    contrib/java/com/zaxxer/HikariCP/5.0.0

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor/3.4.4

    # testing
    contrib/java/junit/junit/4.13.2

    contrib/java/com/github/tomakehurst/wiremock-jre8/2.23.2
    #contrib/java/com/github/tomakehurst/wiremock-jre8/2.26.3

    #2.6.2 to match wiremock-jre8's version
    contrib/java/org/xmlunit/xmlunit-core/2.8.2
    contrib/java/org/xmlunit/xmlunit-legacy/2.6.2
    contrib/java/org/xmlunit/xmlunit-placeholders/2.6.2

    contrib/java/ru/yandex/qatools/embed/postgresql-embedded/2.10
    contrib/java/de/flapdoodle/embed/de.flapdoodle.embed.process/2.0.5 # /2.1.2
    #contrib/java/org/springframework/cloud/spring-cloud-contract-wiremock/2.1.2 #(!!!!!!)

    contrib/java/com/squareup/okhttp3/okhttp/3.12.0
    contrib/java/com/squareup/okhttp3/mockwebserver/3.12.0
    contrib/java/org/skyscreamer/jsonassert/1.5.0
    contrib/java/org/mockito/mockito-core/4.0.0
    contrib/java/net/bytebuddy/byte-buddy-agent/1.11.22
    contrib/java/net/bytebuddy/byte-buddy/1.11.22

    contrib/java/org/testcontainers/postgresql/1.16.2
    contrib/java/org/testcontainers/testcontainers/1.16.2
)
