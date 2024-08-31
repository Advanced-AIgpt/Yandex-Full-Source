package ru.yandex.alice.paskills.my_alice.blocks.skills;

enum Category {
    BUSINESS_FINANCE("business_finance"),
    COMMUNICATION("communication"),
    CONNECTED_CAR("connected_car"),
    EDUCATION_REFERENCE("education_reference"),
    FOOD_DRINK("food_drink"),
    GAMES_TRIVIA_ACCESSORIES("games_trivia_accessories"),
    HEALTH_FITNESS("health_fitness"),
    KIDS("kids"),
    LIFESTYLE("lifestyle"),
    LOCAL("local"),
    MOVIES_TV("movies_tv"),
    MUSIC_AUDIO("music_audio"),
    NEWS("news"),
    PRODUCTIVITY("productivity"),
    SHOPPING("shopping"),
    SMART_HOME("smart_home"),
    TRAVEL_TRANSPORTATION("travel_transportation"),
    UTILITIES("utilities"),
    WEATHER("weather");

    private final String name;

    Category(String value) {
        this.name = value;
    }

    String getName() {
        return name;
    }

}
