package ru.yandex.alice.kronstadt.core.semanticframes

// TODO: change from enum to list of constant strings
/**
 * Introduction of the enum was a mistake as it consolidates the knowledge of all types of all slots of all frames of all scenarios.
 * Global knowledge means global responsibility
 */

@Deprecated("don't use the enum, introduce simple constants")
enum class SemanticSlotType(val value: String) {
    ACTIVATION_PHRASE("activation_phrase"),
    SOUND_LEVEL("level"),
    NEWS_PROVIDER("news_provider"),
    INGREDIENT("ingredient"),
    INGREDIENT_2("ingredient_2"),
    TIME("time"),
    REWIND_TYPE("rewind_type"),
    REWIND_SIZE("rewind_size"),
    ALLOW_TIME("allow_time"),
    PAYLOAD("payload"),

    // recipes
    RECIPE("recipe"),
    RECIPE_WILDCARD("recipe_wildcard"),
    RECIPE_TAG("recipe_tag"),
    RECIPE_TAG_2("recipe_tag_2"),
    RECIPE_DELIVERY_METHOD("delivery_method"),
    RECIPE_REPEAT_HOW("how"),
    RECIPE_WHICH_INGREDIENT("which"),

    // player
    PLAYER_TYPE("player_type"),
    PLAYER_ACTION_TYPE("player_action_type"),
    PLAYER_ENTITY_TYPE("player_entity_type"),
    ACTIVATION_COMMAND("activation_command"),
    NEWS_SOURCE_SLUG("news_source_slug"),
    PROVIDER("provider"),
    PHOTO_ID("photo_id"),
}
