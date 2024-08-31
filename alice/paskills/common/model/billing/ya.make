JAVA_LIBRARY(model-billing)

OWNER(
    g:paskills
    g:smarttvs
)
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.billing.model SRCDIR src/main/java **/*)

LINT(extended)

# fix some versions to be overriden by DM
PEERDIR(
    contrib/java/com/fasterxml/jackson/core/jackson-annotations/2.12.4
    contrib/java/com/fasterxml/jackson/core/jackson-databind/2.12.4
    contrib/java/jakarta/validation/jakarta.validation-api/2.0.2
    library/java/annotations
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    model-billing
    test-results
    src/test/test-results
)


END()
