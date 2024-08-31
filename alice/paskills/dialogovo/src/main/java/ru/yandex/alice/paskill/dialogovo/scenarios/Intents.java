package ru.yandex.alice.paskill.dialogovo.scenarios;

import ru.yandex.alice.kronstadt.core.IrrelevantResponse;

public class Intents {

    public static final String EXTERNAL_SKILL_ACTIVATE = "external_skill.activate";
    public static final String EXTERNAL_SKILL_REQUEST = "external_skill.request";
    public static final String EXTERNAL_SKILL_ACC_LINKING_COMPLETE = "external_skill.account_linking_complete";
    public static final String EXTERNAL_SKILL_PURCHASE_COMPLETE = "external_skill.purchase_complete";
    public static final String EXTERNAL_SKILL_PURCHASE_FAILED = "external_skill.purchase_failed";
    public static final String EXTERNAL_SKILL_RETURN_TO_STATION_AFTER_PURCHASE =
            "external_skill.return_to_station_after_purchase";
    public static final String EXTERNAL_SKILL_DEACTIVATE = "external_skill.deactivate";
    public static final String EXTERNAL_SKILL_IRRELEVANT = IrrelevantResponse.IRRELEVANT; //"external_skill.irrelevant";
    public static final String EXTERNAL_SKILL_EXPLICIT_CONTENT_DENY_ACTIVATION =
            "external_skill.explicit_content.deny_activation";
    public static final String EXTERNAL_SKILL_USER_AGREEMENTS_ACCEPTED = "external_skill.user_agreements.accepted";
    public static final String EXTERNAL_SKILL_USER_AGREEMENTS_REJECTED = "external_skill.user_agreements.rejected";

    public static final String EXTERNAL_SKILL_FEEDBACK_REQUEST = "external_skill.feedback_request";
    public static final String EXTERNAL_SKILL_SAVE_FEEDBACK = "external_skill.save_feedback";

    public static final String EXTERNAL_SKILL_SOUND_QUIETER = "external_skill.sound.quieter";
    public static final String EXTERNAL_SKILL_SOUND_LOUDER = "external_skill.sound.louder";
    public static final String EXTERNAL_SKILL_SOUND_SET_LEVEL = "external_skill.sound.set_level";
    public static final String EXTERNAL_SKILL_SOUND_SET_LEVEL_INCORRECT = "external_skill.sound.set_level_incorrect";

    public static final String EXTERNAL_SKILL_DISCOVERY_DECLINE = "external_skill_discovery.suggest.decline";

    public static final String SKILLS_ONBOARDING_SHOW = "skills_onboarding.show";
    public static final String SKILLS_ONBOARDING_NEXT = "skills_onboarding.next";
    public static final String PERSONAL_ASSISTANT_SKILL_RECOMMENDATION
            = "personal_assistant.scenarios.skill_recommendation";
    public static final String GAMES_ONBOARDING_IRRELEVANT = "games_onboarding.irrelevant";

    public static final String ALICE4BUSINESS_DEVICE_LOCK_IRRELEVANT = "alice4business.device_lock.irrelevant";
    public static final String ALICE4BUSINESS_DEVICE_LOCK_UNLOCKED = "alice4business.device_lock.unlocked";

    public static final String EXTERNAL_SKILL_ACTIVATE_NEWS_FROM_PROVIDER =
            "external_skill.activate_news_from_provider";
    public static final String EXTERNAL_SKILL_ACTIVATE_NEWS_BY_SUBSCRIPTION =
            "external_skill.activate_news_by_subscription";
    public static final String EXTERNAL_SKILL_CONTINUE_READ_NEWS_FROM_PROVIDER =
            "external_skill.continue_news_from_provider";
    public static final String EXTERNAL_SKILL_NEWS_SUBSCRIPTION_ACCEPT =
            "external_skill.news_subscription_accept";
    public static final String EXTERNAL_SKILL_NEWS_SUBSCRIPTION_DECLINE =
            "external_skill.news_subscription_decline";
    public static final String EXTERNAL_SKILL_ACTIVATE_KIDS_NEWS =
            "external_skill.activate_kids_news";
    public static final String EXTERNAL_SKILL_NEWS_IRRELEVANT = "external_skill.news.irrelevant";
    public static final String EXTERNAL_SKILL_RADIONEWS_ONBOARDING = "external_skill.news.radionews.onboarding";

    public static final String ALICE_SCENARIOS_GET_PHOTO_FRAME = "alice_scenarios.get_photo_frame";

    public static final String EXTERNAL_SKILL_COLLECT_WIDGET_GALLERY = "external_skill.collect_widget_gallery";

    // любое управление состоянием плейера - старт, стоп и т.д.
    public static final String AUDIO_PLAYER_CONTROL = "external_skill.audio_player.control";
    // пользовательский запрос, который полетел в навык во время владения плейером - но не событие от девайса
    public static final String AUDIO_PLAYER_SKILL_USER_REQUEST = "external_skill.audio_player.user.request";
    // запросы в навык - только события(коллбэки) от девайса
    public static final String AUDIO_PLAYER_SKILL_CALLBACK_REQUEST = "external_skill.audio_player.callback.request";

    // Пользователь пытается активировать продукт через музыкальный фрагмент
    public static final String MUSIC_SKILL_PRODUCT_ACTIVATION_REQUEST =
            "external_skill.product_activation.music.request";
    // Не получилось распознать музыку при активации продукта, поэтому пытаемся еще раз
    public static final String MUSIC_SKILL_PRODUCT_ACTIVATION_RETRY =
            "external_skill.product_activation.music.retry";


    // generic irrelevant intent, used when neither THEREMIN_NOTHING_IRRELEVANT nor EXTERNAL_SKILL_IRRELEVANT
    // can be used
    public static final String IRRELEVANT = IrrelevantResponse.IRRELEVANT;

    private Intents() {
        throw new UnsupportedOperationException();
    }
}
