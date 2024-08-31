package ru.yandex.alice.paskill.dialogovo.providers.recommendation;


import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters;
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker;

public abstract class BaseRandomSelectorRecommendationProvider implements SkillRecommendationsProvider {

    private final SkillProvider skillProvider;
    private final SurfaceChecker surfaceChecker;

    protected BaseRandomSelectorRecommendationProvider(SkillProvider skillProvider, SurfaceChecker surfaceChecker) {
        this.skillProvider = skillProvider;
        this.surfaceChecker = surfaceChecker;
    }

    @Override
    public List<SkillInfo> getSkills(Random random, int n, MegaMindRequest request) {
        return pickRandom(getSkillIdList(request), random, n, request);
    }

    abstract List<String> getSkillIdList(MegaMindRequest request);

    private List<SkillInfo> pickRandom(List<String> source, Random random, int n, MegaMindRequest request) {
        List<String> skillIdsCopy = new ArrayList<>(source);
        Collections.shuffle(skillIdsCopy, random);
        return skillIdsCopy.stream()
                .map(skillProvider::getSkill)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .filter(SkillFilters.VALID_FOR_RECOMMENDATIONS)
                .filter(skillInfo -> surfaceChecker.isSkillSupported(request.getClientInfo(), skillInfo.getSurfaces()))
                .filter(skillInfo -> SkillFilters.EXPLICIT_CONTENT_RECOMMENDATION_FILTER.test(request, skillInfo))
                .limit(n)
                .collect(Collectors.toList());
    }
}
