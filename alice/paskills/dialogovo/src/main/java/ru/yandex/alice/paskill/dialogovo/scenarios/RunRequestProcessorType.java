package ru.yandex.alice.paskill.dialogovo.scenarios;

import java.lang.reflect.Modifier;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType;

import static java.util.stream.Collectors.toMap;

public class RunRequestProcessorType implements ProcessorType {
    public static final RunRequestProcessorType REQUEST_FEEDBACK_FORM =
            new RunRequestProcessorType("REQUEST_FEEDBACK_FORM", true);
    public static final RunRequestProcessorType SAVE_FEEDBACK =
            new RunRequestProcessorType("SAVE_FEEDBACK", true);
    public static final RunRequestProcessorType SAVE_NONSENSE_FEEDBACK =
            new RunRequestProcessorType("SAVE_NONSENSE_FEEDBACK", true);

    // MM SkillsDiscovery Scenario
    public static final RunRequestProcessorType SKILL_SUGGEST_USER_DECLINE =
            new RunRequestProcessorType("SKILL_SUGGEST_USER_DECLINE", false);
    public static final RunRequestProcessorType SKILLS_ONBOARDING_GET_NEXT =
            new RunRequestProcessorType("SKILLS_ONBOARDING_GET_NEXT", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_DISCOVERY_STATION =
            new RunRequestProcessorType("EXTERNAL_SKILL_DISCOVERY_STATION", false);
    public static final RunRequestProcessorType SKILLS_DISCOVERY_GC_WITH_DIV_CARDS =
            new RunRequestProcessorType("SKILLS_DISCOVERY_GC_WITH_DIV_CARDS", false);

    // MM Dialogovo Scenario
    public static final RunRequestProcessorType THEREMIN_PLAY =
            new RunRequestProcessorType("THEREMIN_PLAY", false);
    public static final RunRequestProcessorType WHAT_IS_THEREMIN =
            new RunRequestProcessorType("WHAT_IS_THEREMIN", false);
    public static final RunRequestProcessorType HOW_MANY_THEREMIN_SOUNDS =
            new RunRequestProcessorType("HOW_MANY_THEREMIN_SOUNDS", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_FIXED_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_FIXED_ACTIVATE", true);
    public static final RunRequestProcessorType EXTERNAL_SKILL_FAIRYTALE_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_FAIRYTALE_ACTIVATE", true);
    public static final RunRequestProcessorType EXTERNAL_SKILL_INTENT_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_INTENT_ACTIVATE", true);
    public static final RunRequestProcessorType EXTERNAL_SKILL_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_ACTIVATE", true);
    public static final RunRequestProcessorType GAMES_ONBOARDING_WITH_DIV_CARDS =
            new RunRequestProcessorType("GAMES_ONBOARDING_WITH_DIV_CARDS", false);
    public static final RunRequestProcessorType GAMES_ONBOARDING_STATION =
            new RunRequestProcessorType("GAMES_ONBOARDING_STATION", false);
    public static final RunRequestProcessorType SKILLS_ONBOARDING_STATION =
            new RunRequestProcessorType("SKILLS_ONBOARDING_STATION", true);
    public static final RunRequestProcessorType EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST =
            new RunRequestProcessorType("EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST", true);
    public static final RunRequestProcessorType EXTERNAL_SKILL_DEACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_DEACTIVATE", false);
    public static final RunRequestProcessorType PROCESS_SKILL =
            new RunRequestProcessorType("PROCESS_SKILL", false);
    public static final RunRequestProcessorType NEW_DIALOG_SESSION =
            new RunRequestProcessorType("NEW_DIALOG_SESSION", false);
    public static final RunRequestProcessorType ALICE4BUSINESS_DEVICE_LOCK_ANY_INPUT =
            new RunRequestProcessorType("ALICE4BUSINESS_DEVICE_LOCK_ANY_INPUT", true);
    public static final RunRequestProcessorType ALICE4BUSINESS_DEVICE_LOCK_SERVER_ACTION =
            new RunRequestProcessorType("ALICE4BUSINESS_DEVICE_LOCK_SERVER_ACTION", false);
    public static final RunRequestProcessorType ACCOUNT_LINKING_COMPLETE =
            new RunRequestProcessorType("ACCOUNT_LINKING_COMPLETE", false);
    public static final RunRequestProcessorType ACCOUNT_LINKING_COMPLETE_RETURN_TO_STATION_CALLBACK =
            new RunRequestProcessorType("ACCOUNT_LINKING_COMPLETE_RETURN_TO_STATION_CALLBACK", false);
    public static final RunRequestProcessorType ACCOUNT_LINKING_COMPLETE_SAME_DEVICE_CALLBACK =
            new RunRequestProcessorType("ACCOUNT_LINKING_COMPLETE_SAME_DEVICE_CALLBACK", false);
    public static final RunRequestProcessorType PURCHASE_COMPLETE_SAME_DEVICE_CALLBACK =
            new RunRequestProcessorType("PURCHASE_COMPLETE_SAME_DEVICE_CALLBACK", false);
    public static final RunRequestProcessorType PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK =
            new RunRequestProcessorType("PURCHASE_COMPLETE_RETURN_TO_STATION_CALLBACK", false);
    public static final RunRequestProcessorType USER_AGREEMENTS_ACCEPTED =
            new RunRequestProcessorType("USER_AGREEMENT_ACCEPTED", false);
    public static final RunRequestProcessorType USER_AGREEMENTS_REJECTED =
            new RunRequestProcessorType("USER_AGREEMENT_REJECTED", false);

    // try next as in will be irrelevant on surfaces other than speakers
    public static final RunRequestProcessorType LOUDER =
            new RunRequestProcessorType("LOUDER", false);
    public static final RunRequestProcessorType QUIETER =
            new RunRequestProcessorType("QUIETER", false);
    public static final RunRequestProcessorType SET_SOUND_LEVEL =
            new RunRequestProcessorType("SET_SOUND_LEVEL", false);

    // MM flash_briefing scenario
    public static final RunRequestProcessorType MORNING_SHOW_NEWS =
            new RunRequestProcessorType("MORNING_SHOW_NEWS", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_ACTIVATE", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_FACTS =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_FACTS", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_READ_NEXT =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_READ_NEXT", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_READ_PREV =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_READ_PREV", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_REPEAT_LAST =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_REPEAT_LAST", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_REPEAT_ALL =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_REPEAT_ALL", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_GET_DETAILS =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_GET_DETAILS", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_SEND_DETAILS_LINK =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_SEND_DETAILS_LINK", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_RADIONEWS_ONBOARDING =
            new RunRequestProcessorType("EXTERNAL_SKILL_RADIONEWS_ONBOARDING", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_ACTIVATE_BY_SUBSCRIPTION =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_ACTIVATE_BY_SUBSCRIPTION", true);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_SUBSCRIPTION_CONFIRM =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_SUBSCRIPTION_CONFIRM", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_NEWS_SUBSCRIPTION_DECLINE =
            new RunRequestProcessorType("EXTERNAL_SKILL_NEWS_SUBSCRIPTION_DECLINE", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_KIDS_NEWS_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_KIDS_NEWS_ACTIVATE", false);
    public static final RunRequestProcessorType EXTERNAL_SKILL_KIDS_NEWS_ON_WIDE_REQUEST_SAFE_MODE_ACTIVATE =
            new RunRequestProcessorType("EXTERNAL_SKILL_KIDS_NEWS_ON_WIDE_REQUEST_SAFE_MODE_ACTIVATE", false);

    // audio player callbacks
    public static final RunRequestProcessorType ON_AUDIO_PLAY_STARTED =
            new RunRequestProcessorType("ON_AUDIO_PLAY_STARTED", false);
    public static final RunRequestProcessorType ON_AUDIO_PLAY_STOPPED =
            new RunRequestProcessorType("ON_AUDIO_PLAY_STOPPED", false);
    public static final RunRequestProcessorType ON_AUDIO_PLAY_FINISHED =
            new RunRequestProcessorType("ON_AUDIO_PLAY_FINISHED", false);
    public static final RunRequestProcessorType ON_AUDIO_PLAY_FAILED =
            new RunRequestProcessorType("ON_AUDIO_PLAY_FAILED", false);
    public static final RunRequestProcessorType GET_NEXT_AUDIO_PLAYER_ITEM =
            new RunRequestProcessorType("GET_NEXT_AUDIO_PLAYER_ITEM", false);

    // audio player user intents
    public static final RunRequestProcessorType AUDIO_PLAYER_INTENT_TO_SKILL_PROXY =
            new RunRequestProcessorType("AUDIO_PLAYER_INTENT_TO_SKILL_PROXY", false);
    public static final RunRequestProcessorType AUDIO_PLAYER_REPLAY =
            new RunRequestProcessorType("AUDIO_PLAYER_REPLAY", false);
    public static final RunRequestProcessorType AUDIO_PLAYER_REWIND =
            new RunRequestProcessorType("AUDIO_PLAYER_REWIND", false);
    public static final RunRequestProcessorType AUDIO_PLAYER_PAUSE =
            new RunRequestProcessorType("AUDIO_PLAYER_PAUSE", false);

    // audio player modality
    public static final RunRequestProcessorType AUDIO_PLAYER_MODALITY_RESUME =
            new RunRequestProcessorType("AUDIO_PLAYER_MODALITY_RESUME", true);

    // recipes
    public static final RunRequestProcessorType SELECT_RECIPE =
            new RunRequestProcessorType("SELECT_RECIPE", true);
    public static final RunRequestProcessorType SELECT_RECIPE_CALLBACK =
            new RunRequestProcessorType("SELECT_RECIPE_CALLBACK", true);
    public static final RunRequestProcessorType UNSUPPORTED_SURFACE =
            new RunRequestProcessorType("UNSUPPORTED_SURFACE", false);
    public static final RunRequestProcessorType UNKNOWN_RECIPE =
            new RunRequestProcessorType("UNKNOWN_RECIPE", false);
    public static final RunRequestProcessorType RECIPE_NEXT_STEP =
            new RunRequestProcessorType("RECIPE_NEXT_STEP", true);
    public static final RunRequestProcessorType RECIPE_PREVIOUS_STEP =
            new RunRequestProcessorType("RECIPE_PREVIOUS_STEP", true);
    public static final RunRequestProcessorType STOP_PLAYING_TIMER =
            new RunRequestProcessorType("STOP_PLAYING_TIMER", true);
    public static final RunRequestProcessorType TIMER_ALARM =
            new RunRequestProcessorType("TIMER_ALARM", true);
    public static final RunRequestProcessorType INSIDE_RECIPE =
            new RunRequestProcessorType("INSIDE_RECIPE", true);
    public static final RunRequestProcessorType RECIPE_ONBOARDING =
            new RunRequestProcessorType("RECIPE_ONBOARDING", false);
    public static final RunRequestProcessorType GET_INGREDIENT_LIST =
            new RunRequestProcessorType("GET_IGREDIENT_LIST", true);
    public static final RunRequestProcessorType RECIPE_REPEAT =
            new RunRequestProcessorType("RECIPE_REPEAT", true);
    public static final RunRequestProcessorType STOP_COOKING =
            new RunRequestProcessorType("STOP_COOKING", true);
    public static final RunRequestProcessorType STOP_COOKING_SUGGEST =
            new RunRequestProcessorType("STOP_COOKING_SUGGEST", true);
    public static final RunRequestProcessorType HOW_MUCH_TO_PUT =
            new RunRequestProcessorType("HOW_MUCH_TO_PUT", true);
    public static final RunRequestProcessorType GET_INGREDIENT_LIST_CALLBACK =
            new RunRequestProcessorType("GET_INGREDIENT_LIST_CALLBACK", false);
    public static final RunRequestProcessorType MUSIC_SUGGEST_DECLINE =
            new RunRequestProcessorType("MUSIC_SUGGEST_DECLINE", false);

    // user skill products
    public static final RunRequestProcessorType MUSIC_SKILL_PRODUCT_ACTIVATION =
            new RunRequestProcessorType("MUSIC_SKILL_PRODUCT_ACTIVATION", false);

    public static final RunRequestProcessorType REQUEST_GEOLOCATION_SHARING =
            new RunRequestProcessorType("REQUEST_GEOLOCATION_SHARING", false);

    // photo frame
    public static final RunRequestProcessorType PHOTO_FRAME_GET_STUB_IMAGE =
            new RunRequestProcessorType("PHOTO_FRAME_GET_STUB_IMAGE", false);

    // widget gallery
    public static final RunRequestProcessorType WIDGET_GALLERY_COLLECTOR =
            new RunRequestProcessorType("WIDGET_GALLERY_COLLECTOR", false);
    // teasers
    public static final RunRequestProcessorType TEASERS_COLLECTOR =
            new RunRequestProcessorType("TEASERS_COLLECTOR", false);

    // alice show
    public static final RunRequestProcessorType ALICE_SHOW_SKILL_EPISODE_COLLECTOR =
            new RunRequestProcessorType("ALICE_SHOW_SKILL_EPISODE_COLLECTOR", false);

    private static final Map<String, RunRequestProcessorType> BY_NAME_MAP =
            Stream.of(RunRequestProcessorType.class.getDeclaredFields())
                    .filter(field -> Modifier.isStatic(field.getModifiers())
                            && RunRequestProcessorType.class.equals(field.getType()))
                    .map(field -> {
                        try {
                            return (RunRequestProcessorType) field.get(null);
                        } catch (IllegalAccessException e) {
                            throw new RuntimeException(e);
                        }
                    })
                    .collect(toMap(RunRequestProcessorType::getName, it -> it));

    private final String name;
    /**
     * true if can try lower priority processor on irrelevant answer from current
     */
    private final boolean tryNextOnIrrelevant;

    RunRequestProcessorType(String name, boolean tryNextOnIrrelevant) {
        this.name = name;
        this.tryNextOnIrrelevant = tryNextOnIrrelevant;
    }

    @Nonnull
    @Override
    public String getName() {
        return name;
    }

    @Override
    public boolean isTryNextOnIrrelevant() {
        return tryNextOnIrrelevant;
    }

    public static Optional<RunRequestProcessorType> byName(String name) {
        return Optional.ofNullable(BY_NAME_MAP.get(name));
    }

}
