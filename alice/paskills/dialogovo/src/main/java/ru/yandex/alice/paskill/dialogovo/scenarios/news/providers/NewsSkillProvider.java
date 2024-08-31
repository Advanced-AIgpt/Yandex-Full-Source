package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.util.List;
import java.util.Optional;

import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillActivationPhraseSearcher;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

public interface NewsSkillProvider extends SkillActivationPhraseSearcher {
    List<NewsSkillInfo> findAllActive();

    Optional<NewsSkillInfo> getSkill(String skillId);

    Optional<NewsSkillInfo> getSkillBySlug(String slug);

    boolean isReady();
}
