package ru.yandex.alice.paskill.dialogovo.scenarios.analytics;

import java.util.List;

import javax.annotation.Nonnull;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillRecommendations;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillRecommendations.TSkillRecommendationType;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject;
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType;


public class SkillRecommendationsAnalyticsInfoObject extends AnalyticsInfoObject {

    private final List<String> skillIds;
    private final RecommendationType recommendationType;

    public SkillRecommendationsAnalyticsInfoObject(List<String> skillIds, RecommendationType recommendationType) {
        super("external_skill.recommendation", recommendationType.name(),
                String.format("Рекоммендация навыков skillIds =[%s]", skillIds));
        this.skillIds = skillIds;
        this.recommendationType = recommendationType;
    }

    public List<String> getSkillIds() {
        return skillIds;
    }

    public RecommendationType getRecommendationType() {
        return recommendationType;
    }

    @Nonnull
    @Override
    public TObject.Builder fillProtoField(@Nonnull TObject.Builder protoBuilder) {
        var payload = TSkillRecommendations.newBuilder()
                .addAllSkillId(skillIds)
                .setSkillRecommendationType(getSkillRecommendationType(recommendationType));
        return protoBuilder.setSkillRecommendations(payload);
    }

    private static TSkillRecommendationType getSkillRecommendationType(RecommendationType recommendationType) {
        switch (recommendationType) {
            case GAMES_ONBOARDING:
                return TSkillRecommendationType.GamesOnboardingRecommnedation;
            default:
                return TSkillRecommendationType.GamesOnboardingRecommnedation;
        }
    }
}
