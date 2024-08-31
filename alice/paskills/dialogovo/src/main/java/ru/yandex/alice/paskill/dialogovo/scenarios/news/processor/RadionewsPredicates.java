package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;

import java.util.function.Predicate;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.utils.VerbosePredicate;

public interface RadionewsPredicates {

    Predicate<MegaMindRequest<DialogovoState>> COUNTRY =
            VerbosePredicate.logMismatch(
                    "COUNTRY_RUSSIA",
                    RunRequestProcessor.COUNTRY_RUSSIA_IF_SPECIFIED);

    Predicate<MegaMindRequest<DialogovoState>> SURFACE =
            VerbosePredicate.logMismatch(
                    "SURFACE",
                    RunRequestProcessor.IS_SMART_SPEAKER_OR_TV
                            .or(RunRequestProcessor.IS_PP)
                            .or(RunRequestProcessor.IS_NAVI_OR_MAPS));
}
