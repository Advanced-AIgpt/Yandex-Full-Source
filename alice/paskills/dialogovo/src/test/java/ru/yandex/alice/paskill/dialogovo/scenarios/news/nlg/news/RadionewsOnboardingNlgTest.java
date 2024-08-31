package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.List;
import java.util.Random;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.config.E2EConfigProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest(classes = {E2EConfigProvider.class})
class RadionewsOnboardingNlgTest {

    @Autowired
    private Phrases phrases;

    @Autowired
    private FlashBriefingVoiceButtonsFactory voiceButtonFactory;

    @Autowired
    private FlashBriefingSuggestsFactory suggestsFactory;

    private final Random random = new Random(0);

    @Test
    void testOne() {
        RadionewsOnboardingNlg nlg = getRadionewsOnboardingNlg(List.of("Вести ФМ"));

        Nlg render = nlg.render();

        assertEquals(
                render.getTts().toString(),
                "О, мне есть, что предложить. Например, новости Вести ФМ. Включаю?");
    }

    @Test
    void testTwo() {
        RadionewsOnboardingNlg nlg = getRadionewsOnboardingNlg(List.of("Вести ФМ", "Коммерсант ФМ"));

        Nlg render = nlg.render();

        assertEquals(
                render.getTts().toString(),
                "Оу, у меня есть радионовости в ассортименте: новости от Вести ФМ или Коммерсант ФМ - включить?");
    }

    @Test
    void testThree() {
        RadionewsOnboardingNlg nlg = getRadionewsOnboardingNlg(List.of("Вести ФМ", "Коммерсант ФМ", "Хит ФМ"));

        Nlg render = nlg.render();

        assertEquals(
                render.getTts().toString(),
                "Оу, у меня есть радионовости в ассортименте: новости от Вести ФМ или Коммерсант ФМ. А еще есть " +
                        "новости Хит ФМ - включить?");
    }

    private RadionewsOnboardingNlg getRadionewsOnboardingNlg(List<String> providers) {
        return new RadionewsOnboardingNlg(
                phrases,
                random,
                providers
                        .stream()
                        .map(name -> NewsSkillInfo.builder()
                                .slug(name)
                                .name(name)
                                .build())
                        .collect(Collectors.toList()),
                voiceButtonFactory,
                suggestsFactory,
                ClientInfo.builder().build());
    }

}
