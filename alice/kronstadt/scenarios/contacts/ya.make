JAVA_LIBRARY(contacts-scenario)

OWNER(g:paskills)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/apache/httpcomponents/httpclient

    alice/kronstadt/core
    alice/kronstadt/scenarios/contacts/src/main/proto

    apphost/api/service/java/apphost
    apphost/lib/proto_answers
    alice/paskills/common/apphost-http-request
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.scenarios.contacts SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -java-parameters
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(
    contrib/java/com/google/protobuf/protobuf-javalite
)

END()

RECURSE_FOR_TESTS(
    it2
)
