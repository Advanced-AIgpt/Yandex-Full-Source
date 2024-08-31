package ru.yandex.alice.paskill.dialogovo.service;

import java.util.Random;

import javax.annotation.Nullable;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.kronstadt.core.layout.TextWithTts;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.TextTtsModifier;

@Component
public class ProactiveSkillResponseWrapper {

    private final Phrases phrases;

    public ProactiveSkillResponseWrapper(Phrases phrases) {
        this.phrases = phrases;
    }

    public void prependActivationMessage(
            TextWithTts skillName,
            @Nullable String skillResponseText,
            TextCard.Builder textCardBuilder,
            SkillProcessResult.Builder skillProcessResultBuilder
    ) {
        final String messageKey = containsStopWord(skillResponseText)
                ? "skill_activation.station.without_stop_word"
                : "skill_activation.station.with_stop_word";
        TextWithTts message = phrases.getTextWithTts(messageKey, skillName);
        textCardBuilder.prependText(message.getText(), "\n", true);
        skillProcessResultBuilder.prependTts(message.getTts(), null, "\n");
    }

    private String appendExitSuggest(String originalText, String exitSuggest) {
        if (containsStopWord(originalText)) {
            return originalText;
        }

        var strippedText = originalText.stripTrailing();
        final String appendedText;
        if (strippedText.isEmpty()) {
            appendedText = exitSuggest;
        } else {
            var missedDot = Character.isLetter(strippedText.charAt(strippedText.length() - 1)) ? "." : "";
            var b = new StringBuffer(strippedText.length() + 2 + exitSuggest.length());
            appendedText = b.append(strippedText).append(missedDot).append(" ").append(exitSuggest).toString();
        }
        return appendedText;
    }

    public void appendExitSuggest(TextTtsModifier response, Random random, String skillName) {
        String exitSuggest = phrases.getRandom("station.exit_suggest", random, skillName);
        response.setText(appendExitSuggest(response.getText(), exitSuggest));
        if (response.getTts() != null) {
            response.setTts(appendExitSuggest(response.getTts(), exitSuggest));
        }
    }


    private boolean containsStopWord(@Nullable String text) {
        return text != null && text.toLowerCase().contains("хватит");
    }

}
