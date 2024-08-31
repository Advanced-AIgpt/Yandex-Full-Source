package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg;

import java.util.List;
import java.util.Random;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotEntityTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.utils.LogoUtils;

@Component
public class FlashBriefingSuggestsFactory {

    private final Phrases phrases;

    FlashBriefingSuggestsFactory(Phrases phrases) {
        this.phrases = phrases;
    }

    public Button repeatAllSuggestConfirm() {
        return Button.simpleTextWithTextDirective(phrases.get("news.content.suggest.repeat.all.repeat.confirm"));
    }

    public Button repeatAllSuggest() {
        return Button.simpleTextWithTextDirective(phrases.get("news.content.suggest.repeat"));
    }

    public Button repeatAllSuggestDecline() {
        return Button.simpleTextWithTextDirective(phrases.get("news.content.suggest.repeat.all.repeat.decline"));
    }

    public Button next(Random random) {
        return Button.simpleTextWithTextDirective(phrases.getRandom("news.content.suggest.next", random));
    }

    public Button provider(NewsSkillInfo skillInfo, ClientInfo clientInfo) {
        var activateNewsProviderSlot = SemanticFrameSlot.create(
                SemanticSlotType.NEWS_PROVIDER.getValue(),
                skillInfo.getName());

        var activateNewsSourceSlugSlot = SemanticFrameSlot.create(
                SemanticSlotType.NEWS_SOURCE_SLUG.getValue(),
                skillInfo.getSlug(),
                SemanticSlotEntityType.CUSTOM_NEWS_SOURCE);

        var skillActivationSourceSlot = SemanticFrameSlot.create(
                SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE,
                ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL.value(),
                SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE);

        SemanticFrame startRadionewsFrame = SemanticFrame.create(
                SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE,
                activateNewsProviderSlot,
                activateNewsSourceSlugSlot,
                skillActivationSourceSlot);

        if (clientInfo.supportsDivCards() && skillInfo.getLogoUrl() != null) {
            return Button.simpleTextWithSemanticFrameWithImage(
                    phrases.get("news.content.suggest.news.provider", List.of(skillInfo.getName())),
                    startRadionewsFrame,
                    LogoUtils.makeLogo(skillInfo.getLogoUrl(), ImageAlias.MOBILE_LOGO_X2));
        } else {
            return Button.simpleTextWithSemanticFrame(phrases.get("news.content.suggest.news.provider",
                    List.of(skillInfo.getName())),
                    startRadionewsFrame);
        }
    }

    public Button details(Random random) {
        return Button.simpleTextWithTextDirective(phrases.getRandom("news.content.suggest.details", random));
    }

    public Button sendDetailsLink(Random random) {
        return Button.simpleTextWithTextDirective(
                phrases.getRandom("news.content.suggest.send.details.link", random));
    }
}
