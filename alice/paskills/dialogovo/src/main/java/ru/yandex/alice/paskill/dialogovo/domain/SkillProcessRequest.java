package ru.yandex.alice.paskill.dialogovo.domain;

import java.time.Instant;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Random;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Data;

import ru.yandex.alice.kronstadt.core.WithExperiments;
import ru.yandex.alice.kronstadt.core.directive.ModroviaCallbackDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.FiltrationLevel;
import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.megamind.protos.common.FrameProto;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TMementoData;
import ru.yandex.alice.paskill.dialogovo.controller.SkillPurchaseCallbackRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.TeasersRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WidgetGalleryRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.show.ShowPullRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct.SkillProductActivationEvent;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ShowUserAgreements;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.UserAgreementsAcceptedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.UserAgreementsRejectedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation.GeolocationSharingAllowedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation.GeolocationSharingRejectedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment.PurchaseCompleteEvent;

import static java.util.Objects.requireNonNull;

@Data
public class SkillProcessRequest implements WithExperiments {
    private final ClientInfo clientInfo;
    private final Optional<LocationInfo> locationInfo;
    @Nullable
    private final String normalizedUtterance;
    @Nullable
    private final String originalUtterance;
    private final Set<String> experiments;
    private final Optional<Session> session;
    private final boolean isDangerousContext;
    private final Optional<FiltrationLevel> filtrationLevel;
    private final SkillInfo skill;
    private final boolean isButtonPress;
    private final boolean voiceSession;
    private final Random random;
    private final boolean accountLinkingCompleteEvent;
    private final boolean mordoviaCallbackEvent;
    @Nullable
    private final ModroviaCallbackDirective mordoviaCallbackDirective;
    private final ActivationSourceType activationSourceType;
    private final boolean testRequest;

    @Nullable
    private final String buttonPayload;

    // empty if view state is not supported (not enabled for skill or not supported by the surface)
    private final Optional<Map<String, Object>> viewState;
    private final Optional<AudioPlayerState> audioPlayerState;
    private final Optional<AudioPlayerEventRequest> audioPlayerEvent;
    private final Optional<SkillPurchaseCallbackRequest> skillPurchaseConfirmEvent;
    private final Optional<PurchaseCompleteEvent> skillPurchaseCompleteEvent;
    private final Optional<SkillProductActivationEvent> skillProductActivationEvent;
    private final Optional<ShowPullRequest> showPullRequest;
    private final Optional<WidgetGalleryRequest> widgetGalleryRequest;
    private final Optional<TeasersRequest> teasersRequest;
    private final Optional<ShowUserAgreements> showUserAgreements;
    private final Optional<GeolocationSharingAllowedEvent> geolocationSharingAllowEvent;
    private final Optional<GeolocationSharingRejectedEvent> geolocationSharingRejectedEvent;
    private final Optional<UserAgreementsAcceptedEvent> userAgreementsAcceptedEvent;
    private final Optional<UserAgreementsRejectedEvent> userAgreementsRejectedEvent;
    private final boolean needToShareGeolocation;
    private final Instant requestTime;
    private final TMementoData mementoData;
    private final List<FrameProto.TTypedSemanticFrame> providableToSkillFrames;

    @SuppressWarnings("ParameterNumber")
    public SkillProcessRequest(ClientInfo clientInfo,
                               Optional<LocationInfo> locationInfo,
                               @Nullable String normalizedUtterance,
                               @Nullable String originalUtterance,
                               @Nullable Set<String> experiments,
                               Optional<Session> session,
                               boolean isDangerousContext,
                               @Nullable FiltrationLevel filtrationLevel,
                               SkillInfo skill,
                               boolean isButtonPress,
                               boolean voiceSession,
                               Random random,
                               boolean accountLinkingCompleteEvent,
                               boolean mordoviaCallbackEvent,
                               @Nullable String buttonPayload,
                               Optional<Map<String, Object>> viewState,
                               @Nullable ModroviaCallbackDirective mordoviaCallbackDirective,
                               @Nullable AudioPlayerState audioPlayerState,
                               @Nullable AudioPlayerEventRequest audioPlayerEvent,
                               ActivationSourceType activationSourceType,
                               @Nullable SkillPurchaseCallbackRequest skillPurchaseConfirmEvent,
                               @Nullable PurchaseCompleteEvent skillPurchaseCompleteEvent,
                               @Nullable SkillProductActivationEvent skillProductActivationEvent,
                               @Nullable ShowPullRequest showPullRequest,
                               @Nullable WidgetGalleryRequest widgetGalleryRequest,
                               @Nullable TeasersRequest teasersRequest,
                               @Nullable GeolocationSharingAllowedEvent geolocationSharingAllowedEvent,
                               @Nullable GeolocationSharingRejectedEvent geolocationSharingRejectedEvent,
                               @Nullable UserAgreementsAcceptedEvent userAgreementsAcceptedEvent,
                               @Nullable UserAgreementsRejectedEvent userAgreementsRejectedEvent,
                               boolean needToShareGeolocation,
                               @Nullable ShowUserAgreements showUserAgreements,
                               boolean testRequest,
                               Instant requestTime,
                               TMementoData mementoData,
                               List<FrameProto.TTypedSemanticFrame> providableToSkillFrames) {

        this.clientInfo = clientInfo;
        this.locationInfo = locationInfo != null ? locationInfo : Optional.empty();
        this.normalizedUtterance = normalizedUtterance;
        this.originalUtterance = originalUtterance;
        this.experiments = experiments != null ? experiments : Set.of();
        this.isDangerousContext = isDangerousContext;
        this.filtrationLevel = Optional.ofNullable(filtrationLevel);
        this.skill = skill;
        this.isButtonPress = isButtonPress;
        this.voiceSession = voiceSession;
        this.random = random;
        this.accountLinkingCompleteEvent = accountLinkingCompleteEvent;
        this.mordoviaCallbackEvent = mordoviaCallbackEvent;
        this.buttonPayload = buttonPayload;
        this.mordoviaCallbackDirective = mordoviaCallbackDirective;
        this.session = session != null ? session : Optional.empty();
        this.viewState = viewState;
        this.audioPlayerState = Optional.ofNullable(audioPlayerState);
        this.audioPlayerEvent = Optional.ofNullable(audioPlayerEvent);
        this.activationSourceType = activationSourceType;
        this.skillPurchaseConfirmEvent = Optional.ofNullable(skillPurchaseConfirmEvent);
        this.skillPurchaseCompleteEvent = Optional.ofNullable(skillPurchaseCompleteEvent);
        this.skillProductActivationEvent = Optional.ofNullable(skillProductActivationEvent);
        this.showPullRequest = Optional.ofNullable(showPullRequest);
        this.widgetGalleryRequest = Optional.ofNullable(widgetGalleryRequest);
        this.teasersRequest = Optional.ofNullable(teasersRequest);
        this.userAgreementsAcceptedEvent = Optional.ofNullable(userAgreementsAcceptedEvent);
        this.userAgreementsRejectedEvent = Optional.ofNullable(userAgreementsRejectedEvent);
        this.showUserAgreements = Optional.ofNullable(showUserAgreements);
        this.geolocationSharingAllowEvent = Optional.ofNullable(geolocationSharingAllowedEvent);
        this.geolocationSharingRejectedEvent = Optional.ofNullable(geolocationSharingRejectedEvent);
        this.needToShareGeolocation = needToShareGeolocation;
        this.requestTime = requireNonNull(requestTime, "requestTime must be provided");
        this.testRequest = testRequest;
        this.mementoData = mementoData;
        this.providableToSkillFrames = providableToSkillFrames;
    }

    public static SkillProcessRequestBuilder builder() {
        return new SkillProcessRequestBuilder();
    }

    public ClientInfo getClientInfo() {
        return clientInfo;
    }

    public String getSkillId() {
        return skill.getId();
    }

    public Optional<String> getNormalizedUtterance() {
        return Optional.ofNullable(normalizedUtterance);
    }

    public Optional<String> getOriginalUtterance() {
        return Optional.ofNullable(originalUtterance);
    }

    public boolean isVoiceSession() {
        return voiceSession;
    }

    public SkillInfo getSkill() {
        return skill;
    }

    public String getApplicationId() {
        return getClientInfo().getUuid();
    }

    @Override
    public Set<String> getExperiments() {
        return experiments;
    }

    public Random getRandom() {
        return random;
    }

    public static class SkillProcessRequestBuilder {
        private ClientInfo clientInfo;
        private Optional<LocationInfo> locationInfo;
        private String normalizedUtterance;
        private String originalUtterance;
        private Set<String> experiments;
        private Optional<Session> session;
        private boolean isDangerousContext;
        private FiltrationLevel filtrationLevel;
        private SkillInfo skill;
        private boolean isButtonPress;
        private boolean voiceSession;
        private Random random;
        private boolean accountLinkingCompleteEvent;
        private boolean mordoviaCallbackEvent;
        private String buttonPayload;
        private Optional<Map<String, Object>> viewState;
        private ModroviaCallbackDirective mordoviaCallbackDirective;
        private AudioPlayerState audioPlayerState;
        private AudioPlayerEventRequest audioPlayerEvent;
        private ActivationSourceType activationSourceType;
        private SkillPurchaseCallbackRequest skillPurchaseConfirmEvent;
        private PurchaseCompleteEvent skillPurchaseCompleteEvent;
        private SkillProductActivationEvent skillProductActivationEvent;
        private ShowPullRequest showPullRequest;
        private WidgetGalleryRequest widgetGalleryRequest;
        private TeasersRequest teasersRequest;
        private GeolocationSharingAllowedEvent geolocationSharingAllowedEvent;
        private GeolocationSharingRejectedEvent geolocationSharingRejectedEvent;
        private UserAgreementsAcceptedEvent userAgreementsAcceptedEvent;
        private UserAgreementsRejectedEvent userAgreementsRejectedEvent;
        private boolean needToShareGeolocation;
        private ShowUserAgreements showUserAgreements;
        private boolean testRequest;
        private Instant requestTime;
        private TMementoData mementoData;
        private List<FrameProto.TTypedSemanticFrame> providableToSkillFrames;

        SkillProcessRequestBuilder() {
            providableToSkillFrames = Collections.emptyList();
        }

        public SkillProcessRequestBuilder clientInfo(ClientInfo clientInfo) {
            this.clientInfo = clientInfo;
            return this;
        }

        public SkillProcessRequestBuilder locationInfo(Optional<LocationInfo> locationInfo) {
            this.locationInfo = locationInfo;
            return this;
        }

        public SkillProcessRequestBuilder normalizedUtterance(String normalizedUtterance) {
            this.normalizedUtterance = normalizedUtterance;
            return this;
        }

        public SkillProcessRequestBuilder originalUtterance(String originalUtterance) {
            this.originalUtterance = originalUtterance;
            return this;
        }

        public SkillProcessRequestBuilder experiments(Set<String> experiments) {
            this.experiments = experiments;
            return this;
        }

        public SkillProcessRequestBuilder session(Optional<Session> session) {
            this.session = session;
            return this;
        }

        public SkillProcessRequestBuilder isDangerousContext(boolean isDangerousContext) {
            this.isDangerousContext = isDangerousContext;
            return this;
        }

        public SkillProcessRequestBuilder filtrationLevel(FiltrationLevel filtrationLevel) {
            this.filtrationLevel = filtrationLevel;
            return this;
        }

        public SkillProcessRequestBuilder skill(SkillInfo skill) {
            this.skill = skill;
            return this;
        }

        public SkillProcessRequestBuilder isButtonPress(boolean isButtonPress) {
            this.isButtonPress = isButtonPress;
            return this;
        }

        public SkillProcessRequestBuilder voiceSession(boolean voiceSession) {
            this.voiceSession = voiceSession;
            return this;
        }

        public SkillProcessRequestBuilder random(Random random) {
            this.random = random;
            return this;
        }

        public SkillProcessRequestBuilder accountLinkingCompleteEvent(boolean accountLinkingCompleteEvent) {
            this.accountLinkingCompleteEvent = accountLinkingCompleteEvent;
            return this;
        }

        public SkillProcessRequestBuilder mordoviaCallbackEvent(boolean mordoviaCallbackEvent) {
            this.mordoviaCallbackEvent = mordoviaCallbackEvent;
            return this;
        }

        public SkillProcessRequestBuilder buttonPayload(String buttonPayload) {
            this.buttonPayload = buttonPayload;
            return this;
        }

        public SkillProcessRequestBuilder viewState(Optional<Map<String, Object>> viewState) {
            this.viewState = viewState;
            return this;
        }

        public SkillProcessRequestBuilder mordoviaCallbackDirective(
                ModroviaCallbackDirective mordoviaCallbackDirective
        ) {
            this.mordoviaCallbackDirective = mordoviaCallbackDirective;
            return this;
        }

        public SkillProcessRequestBuilder audioPlayerState(AudioPlayerState audioPlayerState) {
            this.audioPlayerState = audioPlayerState;
            return this;
        }

        public SkillProcessRequestBuilder audioPlayerEvent(AudioPlayerEventRequest audioPlayerEvent) {
            this.audioPlayerEvent = audioPlayerEvent;
            return this;
        }

        public SkillProcessRequestBuilder activationSourceType(ActivationSourceType activationSourceType) {
            this.activationSourceType = activationSourceType;
            return this;
        }

        public SkillProcessRequestBuilder skillPurchaseConfirmEvent(SkillPurchaseCallbackRequest skillPurchaseEvent) {
            this.skillPurchaseConfirmEvent = skillPurchaseEvent;
            return this;
        }

        public SkillProcessRequestBuilder skillPurchaseCompleteEvent(PurchaseCompleteEvent purchaseCompleteEvent) {
            this.skillPurchaseCompleteEvent = purchaseCompleteEvent;
            return this;
        }

        public SkillProcessRequestBuilder skillProductActivationEvent(
                SkillProductActivationEvent skillProductActivationEvent
        ) {
            this.skillProductActivationEvent = skillProductActivationEvent;
            return this;
        }

        public SkillProcessRequestBuilder showPullRequest(ShowPullRequest showPullRequest) {
            this.showPullRequest = showPullRequest;
            return this;
        }

        public SkillProcessRequestBuilder widgetGalleryRequest(WidgetGalleryRequest widgetGalleryRequest) {
            this.widgetGalleryRequest = widgetGalleryRequest;
            return this;
        }

        public SkillProcessRequestBuilder teasersRequest(TeasersRequest teasersRequest) {
            this.teasersRequest = teasersRequest;
            return this;
        }

        public SkillProcessRequestBuilder geolocationSharingAllowedEvent(
                GeolocationSharingAllowedEvent geolocationSharingAllowedEvent
        ) {
            this.geolocationSharingAllowedEvent = geolocationSharingAllowedEvent;
            return this;
        }

        public SkillProcessRequestBuilder geolocationSharingRejectedEvent(
                GeolocationSharingRejectedEvent geolocationSharingRejectedEvent
        ) {
            this.geolocationSharingRejectedEvent = geolocationSharingRejectedEvent;
            return this;
        }

        public SkillProcessRequestBuilder userAgreementsAcceptedEvent(
                UserAgreementsAcceptedEvent userAgreementsAcceptedEvent
        ) {
            this.userAgreementsAcceptedEvent = userAgreementsAcceptedEvent;
            return this;
        }

        public SkillProcessRequestBuilder userAgreementsRejectedEvent(
                UserAgreementsRejectedEvent userAgreementsRejectedEvent
        ) {
            this.userAgreementsRejectedEvent = userAgreementsRejectedEvent;
            return this;
        }

        public SkillProcessRequestBuilder needToShareGeolocation(boolean needToShareGeolocation) {
            this.needToShareGeolocation = needToShareGeolocation;
            return this;
        }

        public SkillProcessRequestBuilder showUserAgreements(ShowUserAgreements showUserAgreements) {
            this.showUserAgreements = showUserAgreements;
            return this;
        }

        public SkillProcessRequestBuilder testRequest(boolean testRequest) {
            this.testRequest = testRequest;
            return this;
        }

        public SkillProcessRequestBuilder requestTime(Instant requestTime) {
            this.requestTime = requestTime;
            return this;
        }

        public SkillProcessRequestBuilder mementoData(TMementoData mementoData) {
            this.mementoData = mementoData;
            return this;
        }

        public SkillProcessRequestBuilder providableToSkillFrames(
                List<FrameProto.TTypedSemanticFrame> providableToSkillFrames
        ) {
            this.providableToSkillFrames = providableToSkillFrames;
            return this;
        }

        public SkillProcessRequest build() {
            return new SkillProcessRequest(clientInfo, locationInfo, normalizedUtterance, originalUtterance,
                    experiments, session, isDangerousContext, filtrationLevel, skill, isButtonPress, voiceSession,
                    random, accountLinkingCompleteEvent, mordoviaCallbackEvent, buttonPayload, viewState,
                    mordoviaCallbackDirective, audioPlayerState, audioPlayerEvent, activationSourceType,
                    skillPurchaseConfirmEvent, skillPurchaseCompleteEvent, skillProductActivationEvent, showPullRequest,
                    widgetGalleryRequest, teasersRequest, geolocationSharingAllowedEvent,
                    geolocationSharingRejectedEvent,
                    userAgreementsAcceptedEvent,
                    userAgreementsRejectedEvent, needToShareGeolocation, showUserAgreements, testRequest, requestTime,
                    mementoData, providableToSkillFrames);
        }

    }
}
