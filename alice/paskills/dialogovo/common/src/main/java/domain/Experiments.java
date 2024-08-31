package ru.yandex.alice.paskill.dialogovo.domain;

public final class Experiments {
    public static final String ENABLE_NER_FOR_SKILLS = "enable_ner_for_skills";
    public static final String SEND_DEVICE_ID = "dialogovo_send_device_id";
    public static final String DONT_APPLY_ABUSE = "dialogovo_dont_apply_abuse";
    public static final String EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED = "external_skill_fixed_activate_disabled";
    public static final String INTERNAL_MUSIC_PLAYER = "internal_music_player";
    public static final String VOICE_DISCOVERY_SUGGEST = "voice_discovery_suggest";
    public static final String VOICE_DISCOVERY_SUGGEST_THRESHOLD_PREFIX = "voice_discovery_suggest_threshold=";
    public static final String DISABLE_VOLUME_CONTROL = "dialogovo_disable_volume_control";
    // TODO: add disable feature flag/config option when removing this experimental flag
    public static final String REQUEST_FEEDBACK_ON_END_SESSION = "external_skills_feedback_on_end_session";
    public static final String REQUEST_FEEDBACK_ALWAYS = "external_skills_feedback_always";
    public static final String DIALOGOVO_DIV_CARDS_SURFACE_GAMES_ONBOARDING =
            "dialogovo_div_cards_surface_games_onboarding";
    public static final String DIALOGOVO_DIV_CARDS_SURFACE_GAMES_ONBOARDING_SKILL_VOICE_BUTTONS =
            "dialogovo_div_cards_surface_games_onboarding_skill_voice_buttons";
    public static final String USE_SMART_SPEAKER_DEACTIVATE_GRAMMAR = "dialogovo_use_smart_speaker_deactivate_grammar";
    public static final String SUGGEST_EXIT_MORE_OFTEN = "dialogovo_suggest_exit_more_often";
    public static final String DSSM_RERANK_BY_STORE_SCORE = "dialogovo_dssm_rerank_by_store_score";
    public static final String DSSM_RERANK_BY_STORE_SCORE_MIN_DSSM_DISTANCE =
            "dialogovo_dssm_rerank_by_store_score_min_dssm_distance=";
    public static final String FACT_OF_THE_DAY = "fact_of_the_day";
    // Temporary uses as internal dialogovo enable scenario flag
    public static final String SAVE_SKILL_LOGS = "dialogovo_save_skill_logs";
    // global control of budget for postrolls value: [0, 1]
    public static final String RADIONEWS_POSTROLL_PERC_LIMIT = "radionews_postroll_perc_limit=";
    public static final String RADIONEWS_SUBSCRIPTION_SUGGEST_PERC_LIMIT = "radionews_subscription_suggest_perc_limit=";
    public static final String RECIPES_SUGGEST_MUSIC_AFTER_FIRST_STEP = "recipes_suggest_music_after_first_step";
    public static final String RECIPES_ONBOARDING_FIX_RECIPE = "recipes_onboarding_fix_recipe=";
    public static final String RECIPES_ENABLE_ON_SEARCHAPP_AND_BROWSER = "recipes_enable_on_searchapp_and_browser";
    public static final String ACCESSIBLE_PRIVATE_SKILLS = "accessible_private_skills=";
    public static final String USE_SKILL_PURCHASE_ANY_DEVICE = "use_skill_purchase_any_device";
    public static final String ALLOW_RADIONEWS_RESTART = "allow_radionews_restart";
    public static final String RESUME_SESSION_AFTER_PLAYER_STOP_WINDOW_SEC =
            "resume_session_after_player_stop_window_sec=";
    public static final String ACTIVATE_FROM_ANOTHER_SKILL = "dialogovo_activate_from_another_skill";
    public static final String GEOSHARING_ENABLE_TEST_INTERVALS = "geosharing_enable_test_intervals";
    public static final String PLAYER_WITH_STACK_ENGINE_DISABLED = "player_with_stack_engine_disabled";
    public static final String ONBOARDING_STATION_SHOW_SKILL_WITH_DESCRIPTION
            = "onboarding_station_show_skill_with_description";
    public static final String ONBOARDING_STATION_SHOW_SKILL_WITH_DESCRIPTION_AND_NEXT
            = "onboarding_station_show_skill_with_description_and_next";
    public static final String SKILL_AUDIO_RENDER_DATA = "skill_audio_render_data";
    public static final String COLLECT_WIDGET_GALLERY_SKILLS = "collect_widget_gallery_skills";
    public static final String SKILLS_FOR_WINDGET_PREFIX = "skills_for_widget=";
    public static final String COLLECT_TEASERS_SKILLS = "collect_teasers_skills";
    public static final String SKILLS_FOR_TEASERS_PREFIX = "skills_for_teasers=";
    public static final String TEASER_SETTINGS = "teaser_settings";


    private Experiments() {
        throw new UnsupportedOperationException();
    }
}
