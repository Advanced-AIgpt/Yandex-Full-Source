package ru.yandex.alice.paskill.dialogovo.domain;

public final class SkillFeatureFlag {
    public static final String SEND_DEVICE_ID = "send_device_id";
    public static final String AD_IN_SKILLS = "ad_in_skills";
    public static final String MORDOVIA = "mordovia";
    public static final String EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED = "external_skill_fixed_activate_disabled";
    public static final String ALLOW_PRIVATE_SKILL_WITH_FIXED_ACTIVATION =
            "allow_private_skill_with_fixed_activation";
    public static final String SAVE_LOGS = "save_logs";
    public static final String USER_GOZORA = "user_gozora";
    public static final String START_ACCOUNT_LINKING_CUSTOM_ANSWER = "start_account_linking_custom_answer";
    public static final String ALLOW_ACTIVATE_ANOTHER_SKILL = "allow_activate_another_skill";
    public static final String DO_NOT_RESET_STATE_ON_KEEP_STATE = "do_not_reset_state_on_keep_state";
    public static final String CAN_PROCESS_FILTERING_MODE = "can_process_filtering_mode";
    public static final String IGNORE_INVALID_SSL = "no-verify-ssl";
    public static final String ALLOW_ADDITIONAL_VALUES_IN_SLOTS = "allow_additional_values_in_slots";
    public static final String PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK_SCHEME
            = "purchase_complete_return_to_station_callback_scheme";
    public static final String APPMETRICA_EVENTS_TIMESTAMP_COUNTER = "appmetrica_events_timestamp_counter";
    public static final String SEND_SKILL_SERVICE_TICKET_DIRECT = "send_skill_serviсe_ticket_for_direct";
    public static final String OAUTH_VTB_FINGERPRINT = "use_vtb_fingerprint_for_oauth";
    public static final String VTB_BRAND_VOICE = "vtb_brand_voice";
    public static final String DISABLE_ACTIVATE_WITH_REQUEST = "disable_activate_with_request";
    public static final String PERSONALIZED_MORNING_SHOW_ENABLED = "personalized_morning_show_enabled";

    private SkillFeatureFlag() {
        throw new UnsupportedOperationException();
    }
}
