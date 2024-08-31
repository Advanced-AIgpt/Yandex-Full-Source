#include "biometry.h"

#include <cmath>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/serialized_enum.h>
#include <util/string/cast.h>
#include <alice/library/json/json.h>

namespace NAlice::NBiometry {

namespace {

constexpr double EPS = 1e-5;

using TBiometricsScoresWithModeConstArray =
    NDomSchemeRuntime::TConstArray<TSchemeTraits,
                                   NBASSRequest::TMetaConst<TSchemeTraits>::TBiometricsScoresWithModeConst>;

bool HasAllModes(const TBiometricsScoresWithModeConstArray& scoresWithMode) {
    THashSet<TBiometry::EMode> availableModes;
    for (const auto& scoreWithMode : scoresWithMode) {
        TBiometry::EMode mode;
        if (TryFromString(scoreWithMode.Mode(), mode)) {
            availableModes.insert(mode);
        }
    }

    for (const auto& mode : GetEnumAllValues<TBiometry::EMode>()) {
        if (mode == TBiometry::EMode::NoGuest) {
            // this mode is artificial and are not shipped with other scores
            continue;
        }
        if (!availableModes.contains(mode)) {
            return false;
        }
    }
    return true;
}

} // namespace

TBassMetaConst NImpl::ConvertBiometricsScores(const TBiometryScoring& proto) {
    NJson::TJsonValue meta;
    meta["biometrics_scores"] = JsonFromProto(proto);
    return TBassMetaConst(NSc::TValue::FromJsonValue(meta));
}

TBassMetaConst NImpl::CreateEmptyMetaWithoutBiometricScores() {
    NJson::TJsonValue meta;
    meta.SetType(NJson::JSON_MAP);
    return TBassMetaConst(NSc::TValue::FromJsonValue(meta));
}

TBiometry::TBiometry(const TBiometryScoring& proto, IDelegate& delegate, EMode mode)
    : TBiometry(NImpl::ConvertBiometricsScores(proto).Scheme(), delegate, mode) {}

TBiometry::TBiometry(IDelegate& delegate, EMode mode)
    :TBiometry(NImpl::CreateEmptyMetaWithoutBiometricScores().Scheme(), delegate, mode) {}

TBiometry::TBiometry(const TBassMetaConstScheme& meta, IDelegate& delegate, EMode mode)
    : Delegate(delegate)
{
    HasBiometryScores = HasNonEmptyBiometricsScores(meta);

    if (!HasBiometryScores) {
        return;
    }

    const auto biometricsScores = meta->BiometricsScores();

    if (biometricsScores.HasScoresWithMode()) {
        HasScoresWithMode = true;
        const auto& scoresWithMode = biometricsScores.ScoresWithMode();
        auto originalMode = mode;

        if (mode == TBiometry::EMode::NoGuest) {
            mode = TBiometry::EMode::HighTPR;
        }
        Y_ENSURE(!scoresWithMode.Empty());
        // scoresWithMode is nonempty, because it is checked in HasNonEmptyBiometricsScores
        for (const auto& scoreWithMode : scoresWithMode) {
            if (scoreWithMode.Mode() == ToString(mode)) {
                if (originalMode == TBiometry::EMode::NoGuest) {
                    auto scoresValue = ZeroGuestScore(scoreWithMode.Scores());
                    InitFromScores(TBiometricsScoreConstArray(&scoresValue));
                } else {
                    InitFromScores(scoreWithMode.Scores());
                }
                return;
            }
        }
        // This part is unreachable, because HasNonEmptyBiometricsScores checks that all modes are present
        Y_UNREACHABLE();
    } else if (mode == EMode::NoGuest) {
        // Hack until VOICESERV-2248
        auto scoresValue = ZeroGuestScore(meta->BiometricsScores().Scores());
        InitFromScores(TBiometricsScoreConstArray(&scoresValue));
    } else {
        InitFromScores(meta->BiometricsScores().Scores());
    }
}

void TBiometry::InitFromScores(const TBiometry::TBiometricsScoreConstArray& scores) {
    if (scores.Empty()) {
        HasBiometryScores = false;
        return;
    }

    size_t best = 0;
    double sum = 0;

    for (size_t i = 0; i < scores.Size(); ++i) {
        sum = sum + scores[i].Score();
        if (i > 0 && scores[i].Score() > scores[best].Score()) {
            best = i;
        }
    }

    if (scores[best].Score() >= (1 - sum)) {
        BestUserId = scores[best].UserId();
        BestScore = scores[best].Score();
        IsKnown = true;
    } else {
        BestScore = 1 - sum;
        IsGuest = true;
    }
}

NSc::TValue TBiometry::ZeroGuestScore(const TBiometry::TBiometricsScoreConstArray& scores) {
    NSc::TValue result;
    double sum = 0.0;
    for (const auto& score : scores) {
        sum += score.Score();
    }
    for (const auto& score : scores) {
        NSc::TValue resultScore;
        resultScore["user_id"] = score.UserId();
        if (std::abs(sum) < EPS) {
            resultScore["score"] = 1. / scores.Size();
        } else {
            resultScore["score"] = score.Score() / sum;
        }
        result.Push(resultScore);
    }
    return result;
}

bool TBiometry::HasScores() const {
    return HasBiometryScores;
}

bool TBiometry::HasIdentity() const {
    return IsKnown || IsGuest;
}

bool TBiometry::IsGuestUser() const {
    return IsGuest;
}

bool TBiometry::IsKnownUser() const {
    return IsKnown;
}

bool TBiometry::IsModeUsed() const {
    return HasScoresWithMode;
}

TMaybe<double> TBiometry::GetBestScore() const {
    if (HasBiometryScores) {
        return BestScore;
    }
    return {};
}

TResult TBiometry::GetUserId(TString& userId) {
    if (IsGuest && !BestUserId) {
        if (const auto error = GetGuestId(BestUserId)) {
            return error;
        }
    }
    if (BestUserId.empty()) {
        return TError{TError::EType::Logic, "Empty UID"};
    }
    userId = BestUserId;
    return {};
}

TResult TBiometry::GetUserName(TString& userName) const {
    if (!BestUserId) {
        return TError{TError::EType::Logic, "Cannot get user name for undefined UID"};
    }

    if (IsGuest) {
        return {};
    }

    if (const auto error = Delegate.GetUserName(BestUserId, userName)) {
        return TError{TError::EType::DataSync, TStringBuilder() << "Cannot get user name from DataSync (uid: "
                                                                << BestUserId << "): " << error->Msg};
    }
    return {};
}

TResult TBiometry::GetGuestId(TString& guestId) const {
    TString ownerUid;

    const auto result = Delegate.GetUid();
    if (!result.Defined()) {
        return TError{TError::EType::Logic, "Cannot get owner UID from BlackBox for this user"};
    }
    ownerUid = *result;

    if (const auto error = Delegate.GetGuestId(ownerUid, guestId)) {
        return error;
    }

    return {};
}

// If we work on devices with voiceprints (e.g. Yandex.Station, but not cellphone)
// then it's expected that we have BiometricsScore field (possibly empty, if no voiceprint
// was recorded yet)
bool ShouldHaveBiometricsScores(const TClientFeatures& clientFeatures) {
    return clientFeatures.IsSmartSpeaker() && !clientFeatures.SupportsNoMicrophone();
}

TResult IsValidBiometricsContext(const TClientFeatures& clientFeatures, const TBassMetaConstScheme& meta) {
    if (ShouldHaveBiometricsScores(clientFeatures)) {
        if (!HasBiometricsScores(meta)) {
            return TError{TError::EType::Logic,
                          "BiometricsScores field is expected, but is absent in meta. Probably something's wrong with "
                          "biometrics server"};
        }
        if (meta.BiometricsScores().HasScoresWithMode()) {
            if (!meta.BiometricsScores().ScoresWithMode().Empty() &&
                !HasAllModes(meta.BiometricsScores().ScoresWithMode())) {
                return TError{TError::EType::Logic, "Not all requested modes supported"};
            };
        }
        return {};
    }

    if (HasBiometricsScores(meta)) {
        return TError{
            TError::EType::Logic,
            "BiometricsScores field is not expected, but is present in meta. Probably something's wrong BASS "
            "biometrics configuration"};
    }
    return {};
}

bool HasBiometricsScores(const TBassMetaConstScheme& meta) {
    return meta.HasBiometricsScores() &&
           (meta.BiometricsScores().HasScoresWithMode() || meta.BiometricsScores().HasScores());
}

bool HasNonEmptyBiometricsScores(const TBassMetaConstScheme& meta) {
    if (!HasBiometricsScores(meta)) {
        return false;
    }
    const auto& biometricsScores = meta.BiometricsScores();
    if (biometricsScores.HasScoresWithMode()) {
        const auto& scoresWithMode = biometricsScores.ScoresWithMode();
        if (scoresWithMode.Empty()) {
            return false;
        }
        if (!HasAllModes(scoresWithMode)) {
            return false;
        }
        for (const auto& scoreWithMode : scoresWithMode) {
            if (!scoreWithMode.HasScores() || scoreWithMode.Scores().Empty()) {
                return false;
            }
        }
        return true;
    }
    return biometricsScores.HasScores() && !biometricsScores.Scores().Empty();
}

} // namespace NAlice::NBiometry
