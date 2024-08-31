package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.EndDialogSessionDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;

import static ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.DialogovoUtils.isInSkillTab;

@Component
class ProcessRunSkillRequestService {
    private static final Logger logger = LogManager.getLogger();

    private final SkillLayoutsFactory skillLayoutsFactory;
    private final SkillProvider skillProvider;
    private final Phrases phrases;

    ProcessRunSkillRequestService(
            SkillLayoutsFactory skillLayoutsFactory,
            SkillProvider skillProvider,
            Phrases phrases) {
        this.skillLayoutsFactory = skillLayoutsFactory;
        this.skillProvider = skillProvider;
        this.phrases = phrases;
    }

    public BaseRunResponse activateSkill(
            Context ctx,
            MegaMindRequest<DialogovoState> baseRequest,
            SkillInfo skillInfo,
            Optional<ActivationSourceType> activationSourceTypeO
    ) {
        return activateSkill(skillInfo, new SkillActivationArguments(ctx, baseRequest, activationSourceTypeO));
    }

    public BaseRunResponse activateSkill(
            String skillId,
            SkillActivationArguments arguments
    ) {
        SkillInfo skillInfo = skillProvider.getSkill(skillId)
                .orElseThrow(() -> new AliceHandledException(
                        "Навык не найден", ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Произошла ошибка"));
        return activateSkill(skillInfo, arguments);
    }

    public BaseRunResponse activateSkill(
            SkillInfo skillInfo,
            SkillActivationArguments arguments
    ) {
        Context ctx = arguments.getCtx();
        ctx.getAnalytics().addObject(new SkillAnalyticsInfoObject(skillInfo));

        MegaMindRequest<DialogovoState> baseRequest = arguments.getBaseRequest();
        if (isImmediateActivateApply(baseRequest.getClientInfo())) {
            logger.info("inside on smart speaker with immediate activation");
        }

        // if request with suffix we pass to skill only suffix part which is normalized at that point
        /*Optional<String> originalUtterance =
                semanticFrameType == SemanticFrameType.ALICE_EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST ?
                        skillR.getRequestO() :
                        skillR.getOriginalUtteranceO();*/
        if (!skillInfo.isAccessibleBy(baseRequest.getUserId(), baseRequest)) {
            // приватные навыки не должны активироваться, он что если раньше навык приватным не был, а теперь стал
            // и пользователь нажимает на кнопку в стории? Таки или иначе может возникнуть ситуация, когда пользователь
            // дойдет до сюда. Это последний рубеж и тут мы не должны активировать навык.

            var layout = Layout.builder()
                    .textCard(phrases.getRandom("activate.no_access_to_private_skill", baseRequest.getRandom()))
                    .shouldListen(false);

            baseRequest.getDialogIdO().ifPresent(dialogId ->
                    layout.directives(List.of(new EndDialogSessionDirective(dialogId)))
            );
            ctx.getAnalytics().setIntent("activate_private_skill");
            return new RunOnlyResponse<>(new ScenarioResponseBody<>(
                    layout.build(),
                    Optional.empty(),
                    ctx.getAnalytics().toAnalyticsInfo(),
                    false
            ));
        }

        if (isInSkillTab(baseRequest, skillInfo.getId()) || isImmediateActivateApply(baseRequest.getClientInfo())) {
            return new ApplyNeededResponse(RequestSkillApplyArguments.onActivation(
                    skillInfo.getId(),
                    Optional.ofNullable(arguments.getCommand()),
                    Optional.ofNullable(arguments.getOriginalUtterance()),
                    arguments.getActivationSourceTypeO(),
                    arguments.getPayload(),
                    arguments.getActivationTypedSemanticFrame()
            ));
        } else {
            logger.info("open skill no yet tab, request to open the tab");

            var layout = skillLayoutsFactory.createOpenDialogLayout(
                    baseRequest.isVoiceSession(),
                    skillInfo,
                    Optional.ofNullable(arguments.getCommand()),
                    Optional.ofNullable(arguments.getOriginalUtterance()),
                    arguments.getActivationSourceTypeO(),
                    arguments.getPayload(),
                    arguments.getActivationTypedSemanticFrame(),
                    baseRequest.getServerTime(),
                    baseRequest.getClientInfo());
            // expect_request=false for OpenDialog directive see PASKILLS-4603
            return new RunOnlyResponse<>(new ScenarioResponseBody<>(layout, Optional.empty(),
                    ctx.getAnalytics().toAnalyticsInfo(Intents.EXTERNAL_SKILL_ACTIVATE), false));

        }
    }

    private boolean isImmediateActivateApply(ClientInfo clientInfo) {
        return clientInfo.isYaSmartDevice()
                || clientInfo.isNavigatorOrMaps()
                || clientInfo.isYaAuto()
                || clientInfo.isElariWatch();
    }
}
