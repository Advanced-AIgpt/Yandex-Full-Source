OWNER(
    g:paskills
)

JUNIT5(memento_proto-test)

SIZE(SMALL)

JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento.proto SRCDIR src/java **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    alice/memento/proto
    contrib/java/org/junit/jupiter/junit-jupiter
    contrib/java/org/hamcrest/hamcrest
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

END()
