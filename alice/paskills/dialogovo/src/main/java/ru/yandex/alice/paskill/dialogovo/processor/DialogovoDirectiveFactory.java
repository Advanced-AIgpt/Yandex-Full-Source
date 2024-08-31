package ru.yandex.alice.paskill.dialogovo.processor;

import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerPlayDirective;
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerRewindDirective;
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerStopDirective;
import ru.yandex.alice.kronstadt.core.directive.EndDialogSessionDirective;
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective;
import ru.yandex.alice.kronstadt.core.directive.OpenDialogDirective;
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective;
import ru.yandex.alice.kronstadt.core.directive.PlayerPauseDirective;
import ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective;
import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective;
import ru.yandex.alice.kronstadt.core.directive.TypeTextSilentDirective;
import ru.yandex.alice.kronstadt.core.directive.UpdateDialogInfoDirective;
import ru.yandex.alice.megamind.protos.common.FrameProto;
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective;
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective.EScreenType;
import ru.yandex.alice.paskill.dialogovo.config.SearchAppStyles;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Image;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioMetadata;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.BackgroundMode;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Play;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Rewind;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.ButtonPressDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.NewSessionDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.OnPlayFailedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.OnPlayFinishedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.OnPlayStartedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.OnPlayStoppedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.service.RedirectService;
import ru.yandex.alice.paskill.dialogovo.utils.LogoUtils;

import static java.util.Objects.requireNonNullElse;
import static ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective.TTS_PLAY_PLACEHOLDER_DIRECTIVE;

@Component
public class DialogovoDirectiveFactory {
    private final RequestContext requestContext;
    private final RedirectService redirectService;
    private final SearchAppStyles styles;
    private final Set<String> hostWhiteList = Set.of(
            "music.yandex.ru",
            "music.yandex.net",
            "maps.yandex.ru",
            "maps.yandex.net"
    );

    public DialogovoDirectiveFactory(RequestContext requestContext,
                                     RedirectService redirectService,
                                     SearchAppStyles config) {

        this.requestContext = requestContext;
        this.redirectService = redirectService;
        this.styles = config;
    }

    public UpdateDialogInfoDirective updateDialogInfoDirective(SkillInfo skill) {

        return new UpdateDialogInfoDirective(

                skill.getName(),
                skill.getStoreUrl(),
                //TODO: what to do if no image provided. field is nullable
                LogoUtils.makeLogo(requireNonNullElse(skill.getLogoUrl(), ""), ImageAlias.MOBILE_LOGO_X2),
                skill.getAdBlockId().orElse(null),
                skill.isExternal() ? styles.getExternal() : styles.getInternal(),
                skill.isExternal() ? styles.getExternalDark() : styles.getInternalDark(),
                List.of(new UpdateDialogInfoDirective.MenuItem("Описание навыка", skill.getStoreUrl())),
                "external_skill__update_dialog_info"
        );
    }

    public OpenDialogDirective openDialogDirective(
            SkillInfo skill,
            Optional<String> requestO,
            Optional<String> originalUtteranceO,
            Optional<ActivationSourceType> activationSourceTypeO,
            Optional<String> payload,
            FrameProto.TTypedSemanticFrame activationTypedSemanticFrame
    ) {
        List<MegaMindDirective> directives = new ArrayList<>();

        directives.add(new EndDialogSessionDirective(skill.getId()));
        directives.add(updateDialogInfoDirective(skill));
        requestO.ifPresent(request -> directives.add(new TypeTextSilentDirective(request)));
        directives.add(new NewSessionDirective(
                skill.getId(),
                requestO.orElse(""),
                originalUtteranceO.orElse(""),
                activationSourceTypeO.map(ActivationSourceType::getType),
                payload,
                activationTypedSemanticFrame
        ));

        return new OpenDialogDirective(skill.getId(), directives);
    }

    /**
     * Тут хитрый момент:
     *
     * <pre>{ "title": "title" } -> "type": "SimpleUtterance", "command": "normalized title", "nlu": {...}</pre>
     * <pre>{ "title": "title", "url": "http://..." } -> запрос в вебхук не приходит</pre>
     * <pre>{ "title": "title", "payload": {...} } -> "type": "ButtonPressed", "payload": {}, "nlu": {...}</pre>
     * <pre>{ "title": "title", "payload": {...}, url: "http://..." } -> "type": "ButtonPressed", "payload": {},
     *          "nlu": {...}</pre>
     */
    public List<MegaMindDirective> createButtonDirectives(Optional<String> text, Optional<String> url,
                                                          Optional<String> payload) {
        var res = new ArrayList<MegaMindDirective>();

        if (url.isPresent()) {
            res.add(createLinkDirective(url.get()));
        } else if (text.isPresent()) {
            if (payload.isPresent()) {
                res.add(new TypeTextSilentDirective(text.get()));
            } else {
                res.add(new TypeTextDirective(text.get()));
            }
        }

        if (payload.isPresent()) {
            res.add(new ButtonPressDirective(text.orElse(""), payload.get(), requestContext.getRequestId()));
        }

        return res;
    }

    public OpenUriDirective createLinkDirective(String url) {
        return createLinkDirective(URI.create(url));
    }

    public OpenUriDirective createLinkDirective(URI url) {
        if (hostWhiteList.contains(url.getHost())) {
            return new OpenUriDirective(url.toASCIIString());
        } else {
            return new OpenUriDirective(redirectService.getSafeUrl(url.toASCIIString()));
        }
    }

    private PlayerPauseDirective musicPlayerPauseDirective(boolean smooth) {
        return new PlayerPauseDirective(smooth);
    }

    public List<MegaMindDirective> musicPlayerStopSequence(boolean smooth) {
        return List.of(musicPlayerPauseDirective(smooth), ttsPlayPlaceholderDirective());
    }

    private AudioPlayerStopDirective audioStopDirective() {
        return AudioPlayerStopDirective.INSTANCE;
    }

    public List<MegaMindDirective> audioStopSequence() {
        return List.of(audioStopDirective(), ttsPlayPlaceholderDirective());
    }

    public TtsPlayPlaceholderDirective ttsPlayPlaceholderDirective() {
        return TTS_PLAY_PLACEHOLDER_DIRECTIVE;
    }

    public AudioPlayerPlayDirective audioPlayDirective(Play playAction, String skillId, String skillName) {
        var stream = new AudioPlayerPlayDirective.AudioStream(
                URI.create(playAction.getAudioItem().getStream().getUrl()),
                Math.toIntExact(playAction.getAudioItem().getStream().getOffsetMs()),
                playAction.getAudioItem().getStream().getToken()
        );
        Optional<AudioMetadata> origMeta = playAction.getAudioItem().getMetadata();
        var metadata = new AudioPlayerPlayDirective.AudioMetadata(
                origMeta.flatMap(AudioMetadata::getTitle).orElse(null),
                origMeta.flatMap(AudioMetadata::getSubTitle).orElse(null),
                origMeta.flatMap(AudioMetadata::getArt)
                        .map(Image::getUrl)
                        .map(URI::create)
                        .map(AudioPlayerPlayDirective.Image::new)
                        .orElse(null),
                origMeta.flatMap(AudioMetadata::getBackgroundImage)
                        .map(Image::getUrl)
                        .map(URI::create)
                        .map(AudioPlayerPlayDirective.Image::new)
                        .orElse(null)
        );
        var audioItem = new AudioPlayerPlayDirective.AudioItem(stream, metadata);
        var backgroundMode = playAction.getBackgroundMode().map(BackgroundMode::getDirectiveMode)
                .orElse(TAudioPlayDirective.TBackgroundMode.Ducking);
        var play = new AudioPlayerPlayDirective.Play(audioItem, backgroundMode);
        return new AudioPlayerPlayDirective(
                "external_skill__audio_play__" + skillId,
                play,
                skillName,
                Map.of(
                        MegaMindRequest.DeviceState.PLAYER_STATE_META_SKILL_ID_FIELD, skillId,
                        MegaMindRequest.DeviceState.TRACK_OFFSET_MS_ON_START, String.valueOf(stream.getOffsetMs())
                ),
                new OnPlayStartedCallbackDirective(skillId),
                new OnPlayStoppedCallbackDirective(skillId),
                new OnPlayFinishedCallbackDirective(skillId),
                new OnPlayFailedCallbackDirective(skillId),
                EScreenType.Default
        );
    }

    public AudioPlayerRewindDirective audioRewindDirective(Rewind.Type rewindType, long amountMs) {
        return new AudioPlayerRewindDirective(
                new AudioPlayerRewindDirective.Rewind(rewindType.getDirectiveRewindType(), amountMs)
        );
    }
}
