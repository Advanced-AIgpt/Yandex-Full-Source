package ru.yandex.alice.paskill.dialogovo.scenarios;

import java.util.function.BiPredicate;
import java.util.function.Predicate;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.domain.FiltrationLevel;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.utils.VerboseBiPredicate;
import ru.yandex.alice.paskill.dialogovo.utils.VerbosePredicate;

import static ru.yandex.alice.kronstadt.core.domain.FiltrationLevel.FAMILY_SEARCH;
import static ru.yandex.alice.kronstadt.core.domain.FiltrationLevel.SAFE;

public class SkillFilters {
    public static final Predicate<SkillInfo> VALID_FOR_RECOMMENDATIONS =
            SkillInfo::isValidForRecommendations;

    public static final Predicate<SkillInfo> VALID_FOR_RECOMMENDATIONS_VERBOSE =
            VerbosePredicate.logMismatch(
                    "VALID_FOR_RECOMMENDATIONS",
                    VALID_FOR_RECOMMENDATIONS);

    public static final BiPredicate<MegaMindRequest<?>, SkillInfo> EXPLICIT_CONTENT_RECOMMENDATION_FILTER =
            (request, skillInfo) -> !skillInfo.isExplicitContent()
                    || request.getFiltrationLevel() == FiltrationLevel.NO_FILTER;

    public static final BiPredicate<MegaMindRequest<?>, SkillInfo> CONTAINS_RESTRICTED_EXPLICIT_CONTENT =
            (request, skillInfo) -> (request.getFiltrationLevel() == FAMILY_SEARCH && skillInfo.isExplicitContent()) ||
                    (request.getFiltrationLevel() == SAFE && request.getClientInfo().isYaSmartDevice() &&
                            !("kids".equals(skillInfo.getCategory()) ||
                                    "education_reference".equals(skillInfo.getCategory()) ||
                                    skillInfo.isSafeForChildren()
                            )
                    );

    public static final BiPredicate<MegaMindRequest<?>, SkillInfo> SKILL_CAN_PROCESS_FILTRATION_LEVEL =
            (request, skillInfo) -> request.getFiltrationLevel() != SAFE &&
                    skillInfo.hasFeatureFlag(SkillFeatureFlag.CAN_PROCESS_FILTERING_MODE);

    public static final BiPredicate<MegaMindRequest<?>, SkillInfo> SKILL_CAN_PROCESS_FILTRATION_LEVEL_VERBOSE =
            VerboseBiPredicate.logMatch(
                    "SKILL_CAN_PROCESS_FILTRATION_LEVEL",
                    SKILL_CAN_PROCESS_FILTRATION_LEVEL
            );

    public static final BiPredicate<MegaMindRequest<?>, SkillInfo> HAS_MORDOVIA_VIEW = (request, skillInfo) ->
            request.getClientInfo().isQuasar() &&
                    (skillInfo.hasFeatureFlag(SkillFeatureFlag.MORDOVIA) ||
                            skillInfo.hasUserFeatureFlag(UserFeatureFlag.MORDOVIA));


    private SkillFilters() {
    }
}
