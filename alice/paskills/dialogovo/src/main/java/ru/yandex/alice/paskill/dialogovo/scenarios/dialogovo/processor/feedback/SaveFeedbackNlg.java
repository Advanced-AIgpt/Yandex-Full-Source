package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.EndDialogSessionDirective;
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;

@Component
class SaveFeedbackNlg {

    ScenarioResponseBody<DialogovoState> generateFeedbackReceivedResponse(
            Context ctx,
            String skillId,
            Optional<Session> session,
            Layout.Builder layoutBuilder,
            Instant currentTime,
            boolean shouldListen,
            Optional<DialogovoState> oldDialogovoState
    ) {
        var directives = new ArrayList<MegaMindDirective>();
        Optional<DialogovoState> state;
        final boolean expectRequests;

        boolean isEndSession = session.map(Session::isEnded).orElse(false);
        if (session.isEmpty() || isEndSession) {
            directives.add(new EndDialogSessionDirective(skillId));
            state = Optional.empty();
            expectRequests = false;
        } else {
            //todo mid-scenario callback
            state = session.map(s -> DialogovoState.createSkillState(
                    skillId,
                    s,
                    currentTime.toEpochMilli(),
                    Optional.empty(),
                    oldDialogovoState.flatMap(DialogovoState::getGeolocationSharingStateO)));
            expectRequests = true;
        }

        layoutBuilder.shouldListen(shouldListen);
        layoutBuilder.directives(directives);

        return new ScenarioResponseBody<>(
                layoutBuilder.build(),
                state,
                ctx.getAnalytics().toAnalyticsInfo(),
                expectRequests);
    }

}
