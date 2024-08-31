package ru.yandex.alice.paskill.dialogovo.providers.recommendation;

import java.util.List;
import java.util.Random;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;

public interface SkillRecommendationsProvider {

    /**
     * Returns a list of no more than n skills
     *
     * @param n       maximum number of elements in result
     * @param request
     * @return list of skill ids
     */
    List<SkillInfo> getSkills(Random random, int n, MegaMindRequest request);

}
