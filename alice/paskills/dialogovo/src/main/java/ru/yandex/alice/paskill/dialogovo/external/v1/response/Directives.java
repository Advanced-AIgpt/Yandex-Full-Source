package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Optional;

import javax.validation.Valid;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerAction;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@JsonInclude(NON_ABSENT)
public class Directives {

    @JsonProperty("audio_player")
    private final Optional<@Valid AudioPlayerAction> audioPlayer;

    @JsonProperty("canvas_show")
    private final Optional<WebhookMordoviaShow> canvasShow;

    @JsonProperty("canvas_command")
    private final Optional<WebhookMordoviaCommand> canvasCommand;

    @JsonProperty("start_account_linking")
    private final Optional<StartAccountLinking> startAccountLinking;

    @JsonProperty("start_purchase")
    private final Optional<@Valid StartPurchase> startPurchase;

    @JsonProperty("confirm_purchase")
    private final Optional<@Valid ConfirmPurchase> confirmPurchase;

    @JsonProperty("activate_skill_product")
    private final Optional<@Valid ActivateSkillProduct> activateSkillProduct;

    @JsonProperty("request_geolocation")
    private final Optional<@Valid RequestGeolocation> requestGeolocation;

    @JsonProperty("show_user_agreements")
    private final Optional<@Valid ShowUserAgreements> showUserAgreements;
}
