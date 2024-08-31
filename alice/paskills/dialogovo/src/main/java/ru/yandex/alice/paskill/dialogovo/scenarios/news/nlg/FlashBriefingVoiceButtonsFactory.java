package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotEntityTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsGetDetailsDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsReadNextDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsReadPrevDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsRepeatAllDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsRepeatLastDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsSendDetailsLinkDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsSubscriptionConfirmDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsSubscriptionDeclineDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

@Component
public class FlashBriefingVoiceButtonsFactory {

    private static final ActionRef.NluHint NLU_HINT_NEXT_NEWS =
            new ActionRef.NluHint(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_NEXT);
    private static final ActionRef.NluHint NLU_HINT_PREV_NEWS =
            new ActionRef.NluHint(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_PREV);
    public static final ActionRef.NluHint NLU_HINT_CONFIRM =
            new ActionRef.NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_CONFIRM);
    public static final ActionRef.NluHint NLU_HINT_DECLINE =
            new ActionRef.NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE);
    public static final ActionRef.NluHint NLU_HINT_REPEAT_LAST =
            new ActionRef.NluHint(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_REPEAT_LAST);
    public static final ActionRef.NluHint NLU_HINT_REPEAT_ALL =
            new ActionRef.NluHint(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_REPEAT_ALL);
    public static final ActionRef.NluHint NLU_HINT_DETAILS =
            new ActionRef.NluHint(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_DETAILS);
    public static final ActionRef.NluHint NLU_HINT_SEND_DETAIL_LINK =
            new ActionRef.NluHint(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_SEND_DETAILS_LINK);
    public static final ActionRef.NluHint NLU_HINT_SEND_DETAILS_LINK_CONFIRM =
            new ActionRef.NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_CONFIRM);
    public static final ActionRef.NluHint NLU_HINT_ALICE_COMMON_DECLINE =
            new ActionRef.NluHint(SkillsSemanticFrames.ALICE_COMMON_DECLINE);

    private final Phrases phrases;

    public FlashBriefingVoiceButtonsFactory(Phrases phrases) {
        this.phrases = phrases;
    }

    public ActionRef next(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsReadNextDirective(skillId, feedId, contentId),
                NLU_HINT_NEXT_NEWS);
    }

    public ActionRef repeatLast(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsRepeatLastDirective(skillId, feedId, contentId),
                NLU_HINT_REPEAT_LAST);
    }

    public ActionRef repeatAllSuggestConfirm(String skillId, String feedId) {
        return ActionRef.withCallback(
                new NewsRepeatAllDirective(skillId, feedId),
                NLU_HINT_CONFIRM);
    }

    public ActionRef nextSuggestConfirm(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsReadNextDirective(skillId, feedId, contentId),
                NLU_HINT_CONFIRM);
    }

    public ActionRef sendDetailsLinkConfirm(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsSendDetailsLinkDirective(skillId, feedId, contentId),
                NLU_HINT_SEND_DETAILS_LINK_CONFIRM);
    }

    public ActionRef sendDetailsLink(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsSendDetailsLinkDirective(skillId, feedId, contentId),
                NLU_HINT_SEND_DETAIL_LINK);
    }

    public ActionRef repeatAll(String skillId, String feedId) {
        return ActionRef.withCallback(
                new NewsRepeatAllDirective(skillId, feedId),
                NLU_HINT_REPEAT_ALL);
    }

    public ActionRef repeatLastWithoutContent(String skillId, String feedId) {
        return ActionRef.withCallback(
                new NewsRepeatAllDirective(skillId, feedId),
                NLU_HINT_REPEAT_LAST);
    }

    public ActionRef activateNewsProviderByName(NewsSkillInfo newsSkillInfo,
                                                ActivationSourceType activationSourceType) {

        var activateNewsProviderSlot = SemanticFrameSlot.create(
                SemanticSlotType.NEWS_PROVIDER.getValue(),
                newsSkillInfo.getName());

        var activateNewsSourceSlugSlot = SemanticFrameSlot.create(
                SemanticSlotType.NEWS_SOURCE_SLUG.getValue(),
                newsSkillInfo.getSlug(),
                SemanticSlotEntityType.CUSTOM_NEWS_SOURCE);

        var skillActivationSourceSlot = SemanticFrameSlot.create(
                SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE,
                activationSourceType.value(),
                SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE);

        return ActionRef.withSemanticFrame(
                SemanticFrame.create(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE,
                        activateNewsProviderSlot,
                        activateNewsSourceSlugSlot,
                        skillActivationSourceSlot),
                new ActionRef.NluHint(
                        "activate_news_provider_" + newsSkillInfo.getId(),
                        getActivateNewsProviderPhrases(newsSkillInfo))
        );
    }

    public ActionRef activateNewsProviderWithConfirm(NewsSkillInfo newsSkillInfo,
                                                     ActivationSourceType activationSourceType) {

        var activateNewsProviderSlot = SemanticFrameSlot.create(
                SemanticSlotType.NEWS_PROVIDER.getValue(),
                newsSkillInfo.getName());

        var activateNewsSourceSlugSlot = SemanticFrameSlot.create(
                SemanticSlotType.NEWS_SOURCE_SLUG.getValue(),
                newsSkillInfo.getSlug(),
                SemanticSlotEntityType.CUSTOM_NEWS_SOURCE);

        var skillActivationSourceSlot = SemanticFrameSlot.create(
                SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE,
                activationSourceType.value(),
                SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE);

        return ActionRef.withSemanticFrame(
                SemanticFrame.create(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE,
                        activateNewsProviderSlot,
                        activateNewsSourceSlugSlot,
                        skillActivationSourceSlot),
                NLU_HINT_CONFIRM);
    }

    private List<String> getActivateNewsProviderPhrases(NewsSkillInfo skillInfo) {
        return Stream.concat(
                        skillInfo.getInflectedActivationPhrases()
                                .stream()
                                .flatMap(inflectedActivationPhrase -> phrases.getAll(
                                                "news.content.postroll.next.provider.approve.phrase.prefix",
                                                inflectedActivationPhrase)
                                        .stream()),
                        Stream.of(phrases.get("news.content.suggest.news.provider", List.of(skillInfo.getName()))))
                .collect(Collectors.toList());
    }

    public ActionRef prev(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsReadPrevDirective(skillId, feedId, contentId),
                NLU_HINT_PREV_NEWS);
    }

    public ActionRef details(String skillId, String feedId, String contentId) {
        return ActionRef.withCallback(
                new NewsGetDetailsDirective(skillId, feedId, contentId),
                NLU_HINT_DETAILS);
    }

    public ActionRef subscriptionDecline(NewsSkillInfo skill, NewsFeed feed) {
        return ActionRef.withCallback(
                new NewsSubscriptionDeclineDirective(skill.getId(), feed.getId()),
                NLU_HINT_DECLINE);
    }

    public ActionRef subscriptionConfirm(NewsSkillInfo skill, NewsFeed feed) {
        return ActionRef.withCallback(
                new NewsSubscriptionConfirmDirective(skill.getId(), feed.getId()),
                NLU_HINT_CONFIRM);
    }

    public ActionRef commonDeclineDoNothing() {
        return ActionRef.withSemanticFrame(
                SemanticFrame.create(SkillsSemanticFrames.GENERAL_PROACTIVITY_DISAGREE_DO_NOTHING),
                NLU_HINT_ALICE_COMMON_DECLINE);
    }
}
