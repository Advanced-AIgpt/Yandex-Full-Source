package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.util.Optional;
import java.util.Set;
import java.util.function.BiPredicate;

import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;

class IntentsPredicates {

    private static final Set<String> ALLOWED_GENERAL_YANDEX_INTENTS = Set.of(
            "YANDEX.CONFIRM",
            "YANDEX.HELP",
            "YANDEX.REJECT",
            "YANDEX.REPEAT",
            "YANDEX.WHAT_CAN_YOU_DO",
            "YANDEX.AUTH"
    );
    private static final Set<String> ALLOWED_YANDEX_PLAYER_INTENTS = Set.of(
            "YANDEX.PLAYER.NEXT",
            "YANDEX.PLAYER.PREVIOUS",
            "YANDEX.PLAYER.CONTINUE"
    );
    private static final Set<String> ALLOWED_YANDEX_BOOK_INTENTS = Set.of(
            "YANDEX.BOOK.NAVIGATION.CHAPTER_NEXT",
            "YANDEX.BOOK.NAVIGATION.CHAPTER_PREVIOUS",
            "YANDEX.BOOK.NAVIGATION.CHAPTER_REPLAY",
            "YANDEX.BOOK.CONTINUE",
            "YANDEX.BOOK.NAVIGATION.NEXT",
            "YANDEX.BOOK.NAVIGATION.PREVIOUS",
            "YANDEX.BOOK.PLAY_BOOK",
            "YANDEX.BOOK.NAVIGATION.PLAY_THIS",
            "YANDEX.BOOK.PURCHASE",
            "YANDEX.BOOK.SEARCH"
    );

    public static final BiPredicate<Intent, Optional<SkillProcessRequest>> GENERAL_YANDEX_INTENT_PREDICATE =
            (intent, skillProcessRequest) ->
                    ALLOWED_GENERAL_YANDEX_INTENTS.contains(intent.getName())
                            || allowedPlayerIntent(intent, skillProcessRequest)
                    // возможно стоит подумать о добавлении навыкам категории "аудиопровайдера" или чего-то подобного
                            || ALLOWED_YANDEX_BOOK_INTENTS.contains(intent.getName());

    public static final BiPredicate<Intent, String> SKILL_INTENT_PREDICATE =
            (intent, skillId) -> intent.getName().startsWith(skillId + ".");

    private IntentsPredicates() {

    }

    private static boolean allowedPlayerIntent(Intent intent, Optional<SkillProcessRequest> skillProcessRequestO) {
        return skillProcessRequestO.isPresent()
                && skillProcessRequestO.get().getAudioPlayerState().isPresent()
                && ALLOWED_YANDEX_PLAYER_INTENTS.contains(intent.getName());
    }
}
