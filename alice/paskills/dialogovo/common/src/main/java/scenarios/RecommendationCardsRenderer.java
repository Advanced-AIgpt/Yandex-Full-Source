package ru.yandex.alice.paskill.dialogovo.scenarios;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import lombok.Data;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective;
import ru.yandex.alice.kronstadt.core.layout.div.BackgroundType;
import ru.yandex.alice.kronstadt.core.layout.div.Color;
import ru.yandex.alice.kronstadt.core.layout.div.DivAction;
import ru.yandex.alice.kronstadt.core.layout.div.DivBackground;
import ru.yandex.alice.kronstadt.core.layout.div.DivBody;
import ru.yandex.alice.kronstadt.core.layout.div.DivState;
import ru.yandex.alice.kronstadt.core.layout.div.ImageElement;
import ru.yandex.alice.kronstadt.core.layout.div.Size;
import ru.yandex.alice.kronstadt.core.layout.div.block.Block;
import ru.yandex.alice.kronstadt.core.layout.div.block.FooterBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.Position;
import ru.yandex.alice.kronstadt.core.layout.div.block.SeparatorBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.UniversalBlock;
import ru.yandex.alice.kronstadt.core.semanticframes.ButtonAnalytics;
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance;
import ru.yandex.alice.megamind.protos.common.FrameProto;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillRecommendation;
import ru.yandex.alice.paskill.dialogovo.semanticframes.FixedActivate;
import ru.yandex.alice.paskill.dialogovo.utils.TextUtils;

import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.capitalizeFirst;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithDot;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithoutTerminal;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.htmlEscape;

@Component
public class RecommendationCardsRenderer {

    public SkillsRendererResult renderSkillRecommendationCardClassic(RecommendationCardsRendererContext context) {
        DivBody.DivBodyBuilder divBodyBuilder = DivBody.builder();
        Map<String, ActionRef> actions = new HashMap<>();

        divBodyBuilder.background(List.of(new DivBackground("#FFFFFF", BackgroundType.SOLID)));

        List<Block> blocks = new ArrayList<>();
        blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));

        int itemNumber = 1;
        for (; itemNumber <= context.items.size(); itemNumber++) {
            SkillRecommendation item = context.items.get(itemNumber - 1);
            blocks.add(renderRecommenderSkill(context, item));
            if (context.addVoiceActivation) {
                actions.put("launch_skill_" + item.getSkill().getId(), skillClickVoiceAction(item, context));
            }
        }

        blocks.add(SeparatorBlock.withDelimiter(Size.XS));
        blocks.add(new FooterBlock(
                Color.coloredText("ВСЕ НАВЫКИ", "#0A4DC3"), openUriAction(
                "skill_recommendation__all_skills",
                context.getStoreUrl()))
        );

        divBodyBuilder.states(List.of(new DivState(1, blocks, null)));

        return SkillsRendererResult
                .builder()
                .divBody(divBodyBuilder.build())
                .actions(actions)
                .build();
    }

    private UniversalBlock renderRecommenderSkill(RecommendationCardsRendererContext context,
                                                  SkillRecommendation item) {
        return UniversalBlock.builder()
                .action(skillClickDivAction(context, item))
                .sideElement(new UniversalBlock.SideElement(
                        new ImageElement(item.getLogoAvatarUrl(), 1),
                        Size.S,
                        Position.LEFT
                ))
                .text(Color.coloredText(endWithDot(capitalizeFirst(htmlEscape(item.getSkill().getDescription()))),
                        "#7f7f7f"))
                .textMaxLines(2)
                .title(endWithoutTerminal(capitalizeFirst(htmlEscape(item.getActivation()))))
                .titleMaxLines(2)
                .build();
    }

    private DivAction openUriAction(String action, String url) {
        return new DivAction(
                action,
                "",
                List.of(new OpenUriDirective(url))
        );
    }

    private String getSkillActionLogId(RecommendationCardsRendererContext context, SkillRecommendation item) {
        return "skill_recommendation__" + context.getRecommendationSource() + "__"
                + context.getRecommendationType() + "__" + item.getSkill().getId();
    }

    private DivAction skillClickDivAction(RecommendationCardsRendererContext context, SkillRecommendation item) {
        String logId = getSkillActionLogId(context, item);
        FixedActivate skillFixedActivate = new FixedActivate(item.getSkill().getId(), context.activationSourceType,
                context.activationTypedSemanticFrame);
        return new DivAction(
                logId,
                "",
                new ParsedUtterance(
                        item.getActivation(),
                        skillFixedActivate,
                        new ButtonAnalytics(skillFixedActivate.defaultPurpose(), "")
                )
        );
    }

    private ActionRef skillClickVoiceAction(SkillRecommendation item, RecommendationCardsRendererContext context) {
        return ActionRef.withTypedSemanticFrame(
                item.getActivation(),
                new FixedActivate(
                        item.getSkill().getId(),
                        context.activationSourceType,
                        context.activationTypedSemanticFrame
                ),
                new ActionRef.NluHint("launch_skill_" + item.getSkill().getId(),
                        List.of(TextUtils.capitalizeFirst(item.getSkill().getName()))));
    }

    @Data
    public static class RecommendationCardsRendererContext {
        private final List<SkillRecommendation> items;
        private final String requestId;
        private final RecommendationType recommendationType;
        private final String recommendationSubType;
        private final String recommendationSource;
        private final boolean addVoiceActivation;
        private final ActivationSourceType activationSourceType;
        private final String storeUrl;
        private final FrameProto.TTypedSemanticFrame activationTypedSemanticFrame;

        @SuppressWarnings("ParameterNumber")
        RecommendationCardsRendererContext(List<SkillRecommendation> items, String requestId,
                                           RecommendationType recommendationType, String recommendationSubType,
                                           String recommendationSource, boolean addVoiceActivation,
                                           ActivationSourceType activationSourceType, String storeUrl,
                                           FrameProto.TTypedSemanticFrame activationTypedSemanticFrame) {
            this.items = items;
            this.requestId = requestId;
            this.recommendationType = recommendationType;
            this.recommendationSubType = recommendationSubType;
            this.recommendationSource = recommendationSource;
            this.addVoiceActivation = addVoiceActivation;
            this.activationSourceType = activationSourceType;
            this.storeUrl = storeUrl;
            this.activationTypedSemanticFrame = activationTypedSemanticFrame;
        }

        public static RecommendationCardsRendererContextBuilder builder() {
            return new RecommendationCardsRendererContextBuilder();
        }

        public static class RecommendationCardsRendererContextBuilder {
            private List<SkillRecommendation> items;
            private String requestId;
            private RecommendationType recommendationType;
            private String recommendationSubType;
            private String recommendationSource;
            private boolean addVoiceActivation;
            private ActivationSourceType activationSourceType;
            private String storeUrl;
            private FrameProto.TTypedSemanticFrame activationTypedSemanticFrame;

            RecommendationCardsRendererContextBuilder() {
            }

            public RecommendationCardsRendererContextBuilder items(List<SkillRecommendation> items) {
                this.items = items;
                return this;
            }

            public RecommendationCardsRendererContextBuilder requestId(String requestId) {
                this.requestId = requestId;
                return this;
            }

            public RecommendationCardsRendererContextBuilder recommendationType(RecommendationType recommendationType) {
                this.recommendationType = recommendationType;
                return this;
            }

            public RecommendationCardsRendererContextBuilder recommendationSubType(String recommendationSubType) {
                this.recommendationSubType = recommendationSubType;
                return this;
            }

            public RecommendationCardsRendererContextBuilder recommendationSource(String recommendationSource) {
                this.recommendationSource = recommendationSource;
                return this;
            }

            public RecommendationCardsRendererContextBuilder addVoiceActivation(boolean addVoiceActivation) {
                this.addVoiceActivation = addVoiceActivation;
                return this;
            }

            public RecommendationCardsRendererContextBuilder activationSourceType(
                    ActivationSourceType activationSourceType
            ) {
                this.activationSourceType = activationSourceType;
                return this;
            }

            public RecommendationCardsRendererContextBuilder storeUrl(String storeUrl) {
                this.storeUrl = storeUrl;
                return this;
            }

            public RecommendationCardsRendererContextBuilder activationTypedSemanticFrame(
                    FrameProto.TTypedSemanticFrame activationTypedSemanticFrame
            ) {
                this.activationTypedSemanticFrame = activationTypedSemanticFrame;
                return this;
            }

            public RecommendationCardsRendererContext build() {
                return new RecommendationCardsRendererContext(items, requestId, recommendationType,
                        recommendationSubType, recommendationSource, addVoiceActivation, activationSourceType,
                        storeUrl, activationTypedSemanticFrame);
            }

        }
    }

    public static class SkillsRendererResult {
        private final DivBody divBody;
        private final Map<String, ActionRef> actions;

        SkillsRendererResult(DivBody divBody, Map<String, ActionRef> actions) {
            this.divBody = divBody;
            this.actions = actions;
        }

        public static SkillsRendererResultBuilder builder() {
            return new SkillsRendererResultBuilder();
        }

        public DivBody getDivBody() {
            return this.divBody;
        }

        public Map<String, ActionRef> getActions() {
            return this.actions;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (o == null || getClass() != o.getClass()) {
                return false;
            }

            SkillsRendererResult that = (SkillsRendererResult) o;

            if (!divBody.equals(that.divBody)) {
                return false;
            }
            return actions.equals(that.actions);
        }

        @Override
        public int hashCode() {
            int result = divBody.hashCode();
            result = 31 * result + actions.hashCode();
            return result;
        }

        public static class SkillsRendererResultBuilder {
            private DivBody divBody;
            private Map<String, ActionRef> actions;

            SkillsRendererResultBuilder() {
            }

            public SkillsRendererResultBuilder divBody(DivBody divBody) {
                this.divBody = divBody;
                return this;
            }

            public SkillsRendererResultBuilder actions(Map<String, ActionRef> actions) {
                this.actions = actions;
                return this;
            }

            public SkillsRendererResult build() {
                return new SkillsRendererResult(divBody, actions);
            }

        }
    }
}
