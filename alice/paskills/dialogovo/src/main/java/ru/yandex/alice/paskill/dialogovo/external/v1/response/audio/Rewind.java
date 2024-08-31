package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import javax.validation.constraints.NotNull;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;

import ru.yandex.alice.kronstadt.core.directive.AudioPlayerRewindDirective;

@Data
@AllArgsConstructor
@EqualsAndHashCode(callSuper = true)
public class Rewind extends AudioPlayerAction {

    @NotNull
    private final Type type;
    private final long amountMs;

    public Type getType() {
        return type;
    }

    public long getAmountMs() {
        return amountMs;
    }

    @Override
    public AudioPlayerActionType getAction() {
        return AudioPlayerActionType.REWIND;
    }

    public enum Type {
        FORWARD(AudioPlayerRewindDirective.RewindType.FORWARD),
        BACKWARD(AudioPlayerRewindDirective.RewindType.BACKWARD),
        ABSOLUTE(AudioPlayerRewindDirective.RewindType.ABSOLUTE);

        private final AudioPlayerRewindDirective.RewindType directiveRewindType;

        Type(AudioPlayerRewindDirective.RewindType directiveRewindType) {
            this.directiveRewindType = directiveRewindType;
        }

        public AudioPlayerRewindDirective.RewindType getDirectiveRewindType() {
            return directiveRewindType;
        }
    }
}
