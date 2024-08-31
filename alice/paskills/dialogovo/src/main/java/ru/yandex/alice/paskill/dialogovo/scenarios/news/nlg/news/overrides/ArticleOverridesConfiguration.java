package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides;

import java.util.List;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.kronstadt.core.text.Phrases;

@Configuration
public class ArticleOverridesConfiguration {

    @Bean
    ArticleOverrides articleOverrides(Phrases phrases) {
        return new ArticleOverrides(List.of(new RadioNewsArticleOverride(phrases)));
    }
}
