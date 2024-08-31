OWNER(g:paskills)

JUNIT5(afisha-scenario-ut)

SIZE(SMALL)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.scenarios.afisha SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/kronstadt/scenarios/afisha
    # test dependencies
    contrib/java/org/mockito/kotlin/mockito-kotlin
    contrib/java/org/junit/jupiter/junit-jupiter
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/kronstadt/scenarios/afisha/afisha-scenario.jar
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    afisha-scenario-ut
    test-results
)

END()
