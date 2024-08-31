OWNER(
    g:paskills
)

JUNIT5(tv-main-templates-ut)

SIZE(SMALL)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.divkttemplates.tvmain SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/divkt_templates/tv_main
    contrib/java/org/junit/jupiter/junit-jupiter
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/junit/junit
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
#FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/divkttemplates/tv_main/tv-main-templates.jar
)

JAVAC_FLAGS(
    -parameters
)

END()
