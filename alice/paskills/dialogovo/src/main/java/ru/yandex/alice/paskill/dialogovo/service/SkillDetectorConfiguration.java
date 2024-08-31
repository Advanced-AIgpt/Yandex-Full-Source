package ru.yandex.alice.paskill.dialogovo.service;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;

import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationService;

@Configuration
class SkillDetectorConfiguration {
    @Bean("skillDetector")
    @Primary
    @Qualifier
    public SkillDetector skillDetector(NormalizationService normalizationService, SkillProvider skillProvider) {
        return new SkillDetectorImpl(normalizationService, skillProvider);
    }

    @Bean("newsSkillDetector")
    public SkillDetector newsSkillDetector(NormalizationService normalizationService,
                                           NewsSkillProvider newsSkillProvider) {
        return new SkillDetectorImpl(normalizationService, newsSkillProvider);
    }
}
