package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;
import javax.validation.Valid;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.Censored;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowItemMeta;
import ru.yandex.alice.paskill.dialogovo.utils.TeasersMetaDeserializer;
import ru.yandex.alice.paskill.dialogovo.utils.validator.SizeWithoutTags;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@Builder
@Censored
@AllArgsConstructor
@JsonInclude(NON_ABSENT)
public class Response implements TextTtsModifier {
    @JsonProperty(value = "end_session")
    private final boolean endSession;
    private final Optional<List<@Valid Button>> buttons;
    private final Optional<@Valid Card> card;


    @Censored
    @JsonProperty("show_item_meta")
    private final Optional<@Valid ShowItemMeta> showItemMeta;

    @JsonProperty("widget_gallery_meta")
    private final Optional<@Valid WidgetGalleryMeta> widgetGalleryMeta;

    @JsonProperty("teasers_meta")
    @JsonDeserialize(contentUsing = TeasersMetaDeserializer.class)
    private final Optional<List<@Valid TeaserMeta>> teasersMeta;

    /**
     * Use directives.canvas_show instead
     */
    @Deprecated
    @JsonProperty("canvas_show")
    private final Optional<WebhookMordoviaShow> canvasShow;
    /**
     * Use directives.canvas_command instead
     */
    @Deprecated
    @JsonProperty("canvas_command")
    private final Optional<WebhookMordoviaCommand> canvasCommand;

    private final Optional<@Valid Directives> directives;

    @JsonProperty("should_listen")
    private final Optional<Boolean> shouldListen;

    @Nullable
    @Censored
    @Size(max = 1024)
    private String text;

    @SizeWithoutTags(max = 1024, message = "размер не должен превышать {max} без учета тегов")
    @Censored(isVoice = true)
    @Nullable
    private String tts;

    @JsonProperty("floyd_exit_node")
    @Nullable
    private final String floydExitNode;

    public boolean isEndSession() {
        return endSession;
    }

    public Optional<List<Button>> getButtons() {
        return buttons;
    }

    public Optional<Card> getCard() {
        return card;
    }

    public Optional<ShowItemMeta> getShowItemMeta() {
        return showItemMeta;
    }

    public Optional<WidgetGalleryMeta> getWidgetGalleryMeta() {
        return widgetGalleryMeta;
    }

    public Optional<List<TeaserMeta>> getTeasersMeta() {
        return teasersMeta;
    }

    public Optional<WebhookMordoviaShow> getCanvasShow() {
        return canvasShow;
    }

    public Optional<WebhookMordoviaCommand> getCanvasCommand() {
        return canvasCommand;
    }

    public Optional<Directives> getDirectives() {
        return directives;
    }

    public Optional<Boolean> getShouldListen() {
        return shouldListen;
    }

    @Nullable
    public String getText() {
        return text;
    }

    @Nullable
    public String getTts() {
        return tts;
    }

    @Nullable
    public String getFloydExitNode() {
        return floydExitNode;
    }
}
