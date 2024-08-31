package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import javax.annotation.Nonnull;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillSession;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillSession.TActivationSourceType;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;


public class SessionAnalyticsInfoObject extends AnalyticsInfoObject {
    private final ActivationSourceType activationSourceType;

    public SessionAnalyticsInfoObject(String id, ActivationSourceType activationSourceType) {
        super(id, "external_skill.session", "Сессия внутри навыка");
        this.activationSourceType = activationSourceType;
    }

    public ActivationSourceType getActivationSourceType() {
        return activationSourceType;
    }

    @Nonnull
    @Override
    public TObject.Builder fillProtoField(@Nonnull TObject.Builder protoBuilder) {
        var payload = TSkillSession.newBuilder()
                .setId(getId())
                .setActivationSourceType(getProtoActivationSourceType(activationSourceType));
        return protoBuilder.setSkillSession(payload);
    }

    private static TActivationSourceType getProtoActivationSourceType(ActivationSourceType activationSourceType) {
        switch (activationSourceType) {
            case DISCOVERY:
                return TActivationSourceType.Discovery;
            case GAMES_ONBOARDING:
                return TActivationSourceType.GamesOnboarding;
            case STORE:
                return TActivationSourceType.Store;
            case DEEP_LINK:
                return TActivationSourceType.DeepLink;
            case DIRECT:
                return TActivationSourceType.Direct;
            case MORDA:
                return TActivationSourceType.Morda;
            case DEV_CONSOLE:
                return TActivationSourceType.DevConsole;
            case GET_GREETINGS:
                return TActivationSourceType.GetGreetings;
            case ONBOARDING:
                return TActivationSourceType.Onboarding;
            case POSTROLL:
                return TActivationSourceType.Postroll;
            case TURBOAPP_KIDS:
                return TActivationSourceType.TubroAppKids;
            case RADIONEWS_INTERNAL_POSTROLL:
                return TActivationSourceType.RadionewsInternalPostroll;
            case STORE_ALICE_PRICE_CANDIDATE:
                return TActivationSourceType.StoreAlicePriceCandidate;
            case SOCIAL_SHARING:
                return TActivationSourceType.SocialSharing;
            case UNDETECTED:
                return TActivationSourceType.Undetected;
            default:
                return TActivationSourceType.Undetected;
        }
    }
}
