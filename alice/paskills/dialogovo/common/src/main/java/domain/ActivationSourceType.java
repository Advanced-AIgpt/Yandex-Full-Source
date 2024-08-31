package ru.yandex.alice.paskill.dialogovo.domain;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;

public enum ActivationSourceType implements StringEnum {
    DISCOVERY("discovery"),
    GAMES_ONBOARDING("games_onboarding"),
    SKILLS_STATION_ONBOARDING("skills_station_onboarding"),
    STORE("store"),
    DEEP_LINK("deep_link"),
    DIRECT("direct"),
    MORDA("morda"),
    DEV_CONSOLE("dev_console"),
    FLOYD("floyd"),
    GET_GREETINGS("get_greetings"),
    ONBOARDING("onboarding"),
    POSTROLL("postroll"),
    TURBOAPP_KIDS("turboapp_kids"),
    SKILL_INTENT("skill_intent"),
    UNDETECTED("undetected"),
    RADIONEWS_INTERNAL_POSTROLL("radionews_internal_postroll"),
    STORE_ALICE_PRICE_CANDIDATE("store_alice_price_candidate"),
    NEWS_PROVIDER_SUBSCRIPTION("news_provider_subscription"),
    SCHEDULER("scheduler"),
    SHOW("show"),
    WIDGET_GALLERY("widget_gallery"),
    TEASERS("teasers"),
    PAYMENT("payment"),
    SOCIAL_SHARING("social_sharing");

    public static final StringEnumResolver<ActivationSourceType> R =
            StringEnumResolver.resolver(ActivationSourceType.class);

    private final String type;

    ActivationSourceType(String type) {
        this.type = type;
    }

    public String getType() {
        return type;
    }

    @Override
    public String value() {
        return type;
    }
}
