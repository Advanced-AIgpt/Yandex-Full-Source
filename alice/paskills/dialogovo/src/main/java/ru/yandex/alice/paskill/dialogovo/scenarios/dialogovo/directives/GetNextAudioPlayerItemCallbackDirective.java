package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import com.fasterxml.jackson.annotation.JsonCreator;
import lombok.ToString;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive(value = "get_next_audio_play_item")
@ToString
public class GetNextAudioPlayerItemCallbackDirective extends AudioPlayerCallback {

    @JsonCreator
    public GetNextAudioPlayerItemCallbackDirective(String skillId) {
        super(skillId);
    }
}
