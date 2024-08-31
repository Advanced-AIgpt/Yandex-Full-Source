package ru.yandex.alice.paskill.dialogovo.providers.recommendation;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker;

// todo: score based strategy
class StaticSkillListRecommendationProvider extends BaseRandomSelectorRecommendationProvider {

    private final List<String> defaultSkillIdList;
    private final Map<String, List<String>> experimentalSkillIds;
    private final String experimentFlagPrefix;

    StaticSkillListRecommendationProvider(
            List<String> defaultSkillIdList,
            SkillProvider skillProvider,
            SurfaceChecker surfaceChecker,
            Map<String, List<String>> experimentalSkillIds,
            String experimentFlagPrefix
    ) {
        super(skillProvider, surfaceChecker);
        this.defaultSkillIdList = defaultSkillIdList;
        this.experimentalSkillIds = experimentalSkillIds;
        this.experimentFlagPrefix = experimentFlagPrefix;
    }

    @Override
    List<String> getSkillIdList(MegaMindRequest request) {
        Optional<String> experimentSkillList = request.getExperimentStartWithO(this.experimentFlagPrefix);
        return experimentSkillList
                .flatMap(exp -> Optional.ofNullable(this.experimentalSkillIds.get(
                        exp.replace(experimentFlagPrefix, "")
                )))
                .orElse(defaultSkillIdList);
    }
}
