package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import javax.annotation.Nullable;

public interface TextTtsModifier {
    String getText();

    void setText(String text);

    @Nullable
    String getTts();

    void setTts(@Nullable String tts);

}
