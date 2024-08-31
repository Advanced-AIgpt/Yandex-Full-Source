package ru.yandex.alice.paskill.dialogovo.scenarios;

public class SemanticFrames {
    public static final String ALICE_EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST =
            "alice.external_skill_activate_with_request";
    public static final String EXTERNAL_SKILL_DEACTIVATE = "alice.external_skill_deactivate";
    public static final String EXTERNAL_SKILL_FORCE_DEACTIVATE = "alice.external_skill_force_deactivate";
    public static final String EXTERNAL_SKILL_SMART_SPEAKER_DEACTIVATE =
            "alice.external_skill_deactivate_smart_speaker";
    public static final String EXTERNAL_SKILL_FIXED_ACTIVATE = "alice.external_skill_fixed_activate";
    public static final String EXTERNAL_SKILL_FIXED_ACTIVATE_FAST = "alice.external_skill_fixed_activate.fast";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_MUSIC_FAIRYTALE_OLD =
            "personal_assistant.scenarios.music_fairytale";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_MUSIC_FAIRYTALE_NEW =
            "personal_assistant.scenarios.music_fairy_tale";
    public static final String GAMES_ONBOARDING = "alice.external_skill_games_onboarding";
    public static final String KIDS_GAMES_ONBOARDING = "alice.external_skill_games_onboarding.kids";
    public static final String GAME_SUGGEST = "alice.game_suggest";  // like games onboarding, but from boltalka team
    public static final String EXTERNAL_SKILL_WILDCARD = "alice.external_skill_wildcard";
    public static final String EXTERNAL_SKILL_SESSION_REQUEST = "alice.external_skill_session_request";
    // feedback
    public static final String FEEDBACK_EXCELLENT = "alice.external_skill_feedback_excellent";    // 5
    public static final String FEEDBACK_GOOD = "alice.external_skill_feedback_good";              // 4
    public static final String FEEDBACK_ACCEPTABLE = "alice.external_skill_feedback_acceptable";  // 3
    public static final String FEEDBACK_BAD = "alice.external_skill_feedback_bad";                // 2
    public static final String FEEDBACK_VERY_BAD = "alice.external_skill_feedback_very_bad";      // 1
    // sound
    public static final String SOUND_LOUDER = "personal_assistant.scenarios.sound.louder";
    public static final String SOUND_QUIETER = "personal_assistant.scenarios.sound.quiter";
    public static final String SET_SOUND_LEVEL = "personal_assistant.scenarios.sound.set_level";

    //News
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE = "alice.external_skill.flash_briefing.activate";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_FACTS = "alice.external_skill.flash_briefing.facts";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_NEXT = "alice.external_skill.flash_briefing.next";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_PREV = "alice.external_skill.flash_briefing.prev";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_REPEAT_LAST =
            "alice.external_skill.flash_briefing.repeat_last";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_REPEAT_ALL =
            "alice.external_skill.flash_briefing.repeat_all";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_DETAILS = "alice.external_skill.flash_briefing.details";
    public static final String ALICE_EXTERNAL_FLASH_BRIEFING_SEND_DETAILS_LINK =
            "alice.external_skill.flash_briefing.send_details_link";
    public static final String ALICE_EXTERNAL_SKILL_RADIONEWS_ONBOARDING = "alice.external_skill.radionews.onboarding";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS = "personal_assistant.scenarios.get_news";
    public static final String ALICE_EXTERNAL_SKILL_KIDS_NEWS_ACTIVATE = "alice.external_skill.kids_news.activate";

    //AudioPlayer
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_CONTINUE =
            "personal_assistant.scenarios.player.continue";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_NEXT_TRACK =
            "personal_assistant.scenarios.player.next_track";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_PAUSE = "personal_assistant.scenarios.player.pause";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_PREVIOUS_TRACK =
            "personal_assistant.scenarios.player.previous_track";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REPEAT =
            "personal_assistant.scenarios.player.repeat";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REPLAY =
            "personal_assistant.scenarios.player.replay";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REWIND =
            "personal_assistant.scenarios.player.rewind";
    public static final String PERSONAL_ASSISTANT_SCENARIOS_PLAYER_WHAT_IS_PLAYING =
            "personal_assistant.scenarios.player.what_is_playing";

    // Recipes
    public static final String RECIPE_SELECT_RECIPE = "alice.recipes.select_recipe";
    public static final String RECIPE_SELECT_RECIPE_ELLIPSIS = "alice.recipes.select_recipe.ellipsis";
    public static final String RECIPE_NEXT_STEP = "alice.recipes.next_step";
    public static final String RECIPE_PREVIOUS_STEP = "alice.recipes.previous_step";
    public static final String RECIPE_STOP_PLAYING_TIMER = "alice.recipes.timer_stop_playing";
    public static final String RECIPE_TIMER_ALARM = "alice.recipes.timer_alarm";
    public static final String RECIPE_ONBOARDING = "alice.recipes.onboarding";
    public static final String RECIPE_ONBOARDING_MORE = "alice.recipes.onboarding.more";
    public static final String RECIPE_REPEAT = "alice.recipes.repeat";
    public static final String RECIPE_TELL_INGREDIENT_LIST = "alice.recipes.ingredient_list";
    public static final String RECIPE_STOP_COOKING = "alice.recipes.stop_cooking";
    public static final String RECIPE_STOP_COOKING_SUGGEST = "alice.recipes.stop_cooking_suggest";
    public static final String RECIPE_HOW_MUCH_TO_PUT = "alice.recipes.how_much_to_put";
    public static final String RECIPE_HOW_MUCH_TO_PUT_ELLIPSIS = "alice.recipes.how_much_to_put.ellipsis";

    // Discovery
    public static final String ALICE_EXTERNAL_SKILL_DISCOVERY_GC = "alice.external_skill_discovery.gc";

    // Alice show
    public static final String ALICE_SHOW_GET_SKILL_EPISODE = "alice.external_skill_episode_for_show_request";

    // Geosharing
    public static final String ALICE_EXTERNAL_SKILL_ALLOW_GEOSHARING = "alice.external_skill_allow_geosharing";
    public static final String ALICE_EXTERNAL_SKILL_DO_NOT_ALLOW_GEOSHARING
            = "alice.external_skill_do_not_allow_geosharing";

    // Photo Frame
    public static final String ALICE_SCENARIOS_GET_PHOTO_FRAME = "alice.scenarios.get_photo_frame";
    public static final String ALICE_CENTAUR_COLLECT_CARDS = "alice.centaur.collect_cards";
    public static final String ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW = "alice.centaur.collect_teasers_preview";
    public static final String ALICE_CENTAUR_COLLECT_MAIN_SCREEN = "alice.centaur.collect_main_screen";

    // Other (not related to dialogovo-based scenarios)
    public static final String PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS_MORE =
            "personal_assistant.scenarios.get_news__more";

    private SemanticFrames() {
        throw new UnsupportedOperationException();
    }
}
