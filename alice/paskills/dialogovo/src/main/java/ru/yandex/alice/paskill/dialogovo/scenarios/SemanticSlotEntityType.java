package ru.yandex.alice.paskill.dialogovo.scenarios;

public class SemanticSlotEntityType {

    public static final String INGREDIENT = "Ingredient";
    // recipes
    public static final String RECIPE = "Recipe";
    public static final String RECIPE_TAG = "RecipeTag";
    public static final String RECIPE_DELIVERY_METHOD = "DeliveryMethod";
    public static final String RECIPE_REPETITION_MODIFIER = "RepetitionModifier";
    public static final String CUSTOM_REWIND_TYPE = "custom.rewind_type";
    public static final String CUSTOM_REWIND_SIZE = "custom.rewind_size";
    public static final String SYS_UNITS_TIME = "sys.units_time";
    public static final String CUSTOM_PLAYER_TYPE = "custom.player_type";
    public static final String PLAYER_ACTION_TYPE = "custom.player_action_type";
    public static final String PLAYER_ENTITY_TYPE = "custom.player_entity_type";
    public static final String CUSTOM_NEWS_SOURCE = "custom.news_source";
    public static final String NEWS_PROVIDER = "news_provider";

    private SemanticSlotEntityType() {
        throw new UnsupportedOperationException();
    }

}
