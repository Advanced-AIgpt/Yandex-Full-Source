JAVA_LIBRARY(tv-main-scenario)

OWNER(g:smarttv)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/kronstadt/core
    alice/divkt_templates/tv_main
    alice/protos/data/tv/home
    alice/protos/data/tv/tags
    alice/protos/data/video
    alice/protos/data/tv/app_metrika
    alice/paskills/common/apphost-spring-controller
    alice/kronstadt/scenarios/tv_main/src/main/proto
    entity/recommender/runtime/proto
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.scenarios.tvmain SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -java-parameters
)

JAVAC_FLAGS(
    -parameters
)


END()

RECURSE_FOR_TESTS(
    src/test
    it2
)
