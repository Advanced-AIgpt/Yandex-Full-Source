LIBRARY()

OWNER(
    alkapov
    g:megamind
)

SRCS(
    button_model.cpp
    card_model.cpp
    directive_model.cpp
    model.cpp
    model_serializer.cpp
)

GENERATE_ENUM_SERIALIZATION(button_model.h)
GENERATE_ENUM_SERIALIZATION(card_model.h)
GENERATE_ENUM_SERIALIZATION(directive_model.h)

END()
