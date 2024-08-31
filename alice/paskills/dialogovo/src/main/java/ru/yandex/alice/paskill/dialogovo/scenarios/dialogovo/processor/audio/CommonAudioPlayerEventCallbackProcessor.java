package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import java.util.Optional;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioPlayerCallbackAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioStreamAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.AudioPlayerEventsApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.AudioPlayerCallback;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.AppmetricaCommitArgs;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;

import static java.util.function.Predicate.not;

public abstract class CommonAudioPlayerEventCallbackProcessor<CALLBACK extends AudioPlayerCallback>
        implements CommittingRunProcessor<DialogovoState, AudioPlayerEventsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final SkillRequestProcessor processor;
    private final AppMetricaEventSender appMetricaEventSender;
    private final ObjectMapper objectMapper;

    public CommonAudioPlayerEventCallbackProcessor(SkillProvider skillProvider, SkillRequestProcessor processor,
                                                   AppMetricaEventSender appMetricaEventSender,
                                                   ObjectMapper objectMapper) {
        this.skillProvider = skillProvider;
        this.processor = processor;
        this.appMetricaEventSender = appMetricaEventSender;
        this.objectMapper = objectMapper;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return onCallbackDirective(getCallbackClazz())
                .and(HAS_SKILL_PLAYER_OWNER)
                //TODO: use not(IS_IN_SKILL) only after player modality enables
                .and(not(IS_IN_SKILL).or(IS_IN_SKILL.and(IS_IN_PLAYER_OWNER_SKILL)))
                .test(request);
    }

    protected abstract Class<CALLBACK> getCallbackClazz();

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        AudioPlayerCallback payload = getAudioPlayerCallbackPayload(request);
        // provided by SKILL_PLAYER_OWNER
        String skillIdFromDeviceState = request.getAudioPlayerOwnerSkillIdO().get();
        String callbackInputSkillId = payload.getSkillId();

        if (!skillIdFromDeviceState.equals(callbackInputSkillId)) {
            logger.info("Player skill owner in device_state: [{}] differs from callback: [{}]",
                    skillIdFromDeviceState, callbackInputSkillId);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }

        Optional<SkillInfo> skillO = skillProvider.getSkill(callbackInputSkillId);
        if (skillO.isEmpty()) {
            logger.info("Unable to detect skill id [{}] from audio player callback", callbackInputSkillId);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }
        SkillInfo skillInfo = skillO.get();

        Session session = request.getStateO()
                .flatMap(DialogovoState::getSessionO)
                .map(Session::getNext)
                .orElseGet(() -> Session.create(ActivationSourceType.UNDETECTED, request.getServerTime()));

        boolean isSessionInBackground = request.getStateO().map(DialogovoState::isSessionInBackground)
                .orElse(false);

        MetricaEvent metricaEvent = MetricaEvent.TypedMetricaEvent.create("audio_player_event",
                getAudioPlayerEvent().getType().getMetricaValue(), objectMapper);

        AppmetricaCommitArgs appmetricaArgs = null;
        if (skillInfo.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER)) {
            try {
                appmetricaArgs = appMetricaEventSender.prepareClientEventsForCommit(
                        SourceType.USER,
                        skillInfo.getId(),
                        SkillInfo.Look.EXTERNAL,
                        skillInfo.getEncryptedAppMetricaApiKey(),
                        request.getClientInfo(),
                        request.getLocationInfoO(),
                        request.getServerTime(),
                        Optional.of(session),
                        metricaEvent,
                        Optional.empty(),
                        request.isTest(),
                        skillInfo.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER)
                );
            } catch (Exception ex) {
                logger.error("Failed preparing client app metrica events for commit", ex);
            }

        }

        DialogovoState skillState = DialogovoState.createSkillState(
                skillInfo.getId(),
                session.withAppmetricaEventCounter(session.getAppMetricaEventCounter() + (appmetricaArgs != null ?
                        appmetricaArgs.getReportMessage().getSessions(0).getEventsCount() : 0L)),
                request.getServerTime().toEpochMilli(),
                Optional.empty(),
                isSessionInBackground,
                request.getStateO().flatMap(DialogovoState::getGeolocationSharingStateO));

        // provided by SKILL_PLAYER_OWNER
        MegaMindRequest.DeviceState.AudioPlayer audioPlayerState =
                request.getDeviceStateO().map(MegaMindRequest.DeviceState::getAudioPlayerState).get();
        context.getAnalytics().addObject(new AudioStreamAnalyticsInfoObject(
                audioPlayerState.getToken(),
                audioPlayerState.getOffsetMs()));

        context.getAnalytics()
                .setIntent(Intents.AUDIO_PLAYER_SKILL_CALLBACK_REQUEST)
                .addObject(new SkillAnalyticsInfoObject(skillInfo))
                .addAction(new AudioPlayerCallbackAction(getCallbackAnalyticsAction()))
                .addObject(new SessionAnalyticsInfoObject(
                        session.getSessionId(),
                        session.getActivationSourceType()));

        return new CommitNeededResponse<>(
                new ScenarioResponseBody<>(Layout.builder()
                        .shouldListen(false)
                        .outputSpeech("")
                        .textCard("")
                        .build(),
                        Optional.of(skillState),
                        context.getAnalytics().toAnalyticsInfo(),
                        !isSessionInBackground),
                new AudioPlayerEventsApplyArguments(callbackInputSkillId, appmetricaArgs)
        );
    }

    abstract AudioPlayerCallbackAction.CallbackType getCallbackAnalyticsAction();

    private AudioPlayerCallback getAudioPlayerCallbackPayload(MegaMindRequest<DialogovoState> request) {
        return request.getInput().getDirective(getCallbackClazz());
    }

    @Override
    public CommitResult commit(MegaMindRequest<DialogovoState> request, Context context,
                               AudioPlayerEventsApplyArguments applyArguments) {

        Optional<SkillInfo> skillO = skillProvider.getSkill(applyArguments.getSkillId());
        if (skillO.isEmpty()) {
            logger.info("Unable to detect skill id [{}] from audio player callback on commit",
                    applyArguments.getSkillId());
            return CommitResult.Error;
        }
        SkillInfo skillInfo = skillO.get();

        SkillProcessRequest.SkillProcessRequestBuilder skillProcessRequestBuilder = SkillProcessRequest.builder()
                .skill(skillInfo)
                .experiments(request.getExperiments())
                .clientInfo(request.getClientInfo())
                .session(request.getStateO().flatMap(DialogovoState::getSessionO))
                .viewState(Optional.empty())
                .audioPlayerEvent(getAudioPlayerEvent())
                .requestTime(request.getServerTime());

        Optional<MegaMindRequest.DeviceState.AudioPlayer> audioPlayerStateO =
                request.getDeviceStateO().map(MegaMindRequest.DeviceState::getAudioPlayerState);
        if (audioPlayerStateO.isPresent()) {
            MegaMindRequest.DeviceState.AudioPlayer audioPlayerState = audioPlayerStateO.get();
            skillProcessRequestBuilder.audioPlayerState(new AudioPlayerState(
                    audioPlayerState.getToken(), audioPlayerState.getOffsetMs(), audioPlayerState.getActivityState()));
        }

        processor.processAsync(context, skillProcessRequestBuilder.build());

        return CommitResult.Success;
    }

    protected abstract AudioPlayerEventRequest getAudioPlayerEvent();

    @Override
    public Class<AudioPlayerEventsApplyArguments> getApplyArgsType() {
        return AudioPlayerEventsApplyArguments.class;
    }
}
