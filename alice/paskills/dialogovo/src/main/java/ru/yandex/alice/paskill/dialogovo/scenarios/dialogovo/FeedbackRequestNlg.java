package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.layout.TextWithTts;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.SaveFeedbackDirective;

@Component
public class FeedbackRequestNlg {

    private final Phrases phrases;

    public FeedbackRequestNlg(Phrases phrases) {
        this.phrases = phrases;
    }

    public TextWithTts render(Random random, SkillInfo skill) {
        return phrases.getRandomTextWithTts(
                "feedback_request",
                random,
                List.of(skill.getName()),
                List.of(skill.getNameTts()));
    }

    public List<Button> getSuggests(SkillInfo skill) {
        List<Button> buttons = new ArrayList<>(5);
        for (FeedbackMark mark : FeedbackMark.values()) {
            var feedbackDirective = new SaveFeedbackDirective(skill.getId(), mark);
            var button = Button.simpleText(mark.getTitle(), feedbackDirective);
            buttons.add(button);
        }
        return buttons;
    }
}
