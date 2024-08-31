#pragma once

#include <alice/library/client/client_features.h>
#include <alice/library/client/client_info.h>
#include <alice/library/passport_api/passport_api.h>

#include <alice/bass/libs/request/request.sc.h>
#include <alice/bass/util/error.h>
#include <alice/megamind/protos/common/events.pb.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NBiometry {

enum class EErrorType {
    DataSync /* "data_sync" */,
    Logic /* logic */,
    ModeNotFound /* mode_not_found */,
};

using TError = NBASS::TGenericError<EErrorType>;

using TResult = TMaybe<TError>;

using TBassMetaConstScheme = NBASSRequest::TMetaConst<TSchemeTraits>;
using TBassMetaConst = TSchemeHolder<TBassMetaConstScheme>;

namespace NImpl {

TBassMetaConst ConvertBiometricsScores(const TBiometryScoring& proto);
TBassMetaConst CreateEmptyMetaWithoutBiometricScores();

} // namespace NImpl

class TBiometry {
public:
    struct IDelegate {
        virtual ~IDelegate() = default;

        virtual TMaybe<TString> GetUid() const = 0;

        virtual TResult GetUserName(TStringBuf UserId, TString& userName) const = 0;
        virtual TResult GetGuestId(TStringBuf UserId, TString& guestId) const = 0;
    };

    enum class EMode {
        MaxAccuracy /* "max_accuracy" */,
        HighTPR /* "high_tpr" */,
        HighTNR /* "high_tnr" */,
        // Special mode for selecting best registered user (e. g. for remove)
        NoGuest /* "_no_guest" */
    };

    TBiometry(const TBassMetaConstScheme& meta, IDelegate& delegate, EMode mode);
    TBiometry(const TBiometryScoring& proto, IDelegate& delegate, EMode mode);
    TBiometry(IDelegate& delegate, EMode mode); // Special case when you have no TBiometryScoring proto at all

    bool HasScores() const;
    bool HasIdentity() const;
    bool IsKnownUser() const;
    bool IsGuestUser() const;

    bool IsModeUsed() const;

    TMaybe<double> GetBestScore() const;

    TResult GetUserId(TString& userId);
    TResult GetUserName(TString& userName) const;

private:
    using TBiometricsScoreConstArray =
        NDomSchemeRuntime::TConstArray<TSchemeTraits, NBASSRequest::TMetaConst<TSchemeTraits>::TBiometricsScoreConst>;

    void InitFromScores(const TBiometricsScoreConstArray& scores);
    static NSc::TValue ZeroGuestScore(const TBiometricsScoreConstArray& scores);

    TResult GetGuestId(TString& guestId) const;

    IDelegate& Delegate;

    bool HasBiometryScores = false;
    bool IsKnown = false;
    bool IsGuest = false;

    // TODO(@thefacetak): remove once scores are always with mode
    bool HasScoresWithMode = false;

    double BestScore = 0.0;

    TString BestUserId;
};

// If we work on devices with voiceprints (e.g. Yandex.Station, but not cellphone)
// then it's expected that we have BiometricsScore field (possibly empty, if no voiceprint
// was recorded yet)
bool ShouldHaveBiometricsScores(const TClientFeatures& clientFeatures);

// Check that we have BiometricsScore field if we need it and don't have if we don't.
TResult IsValidBiometricsContext(const TClientFeatures& clientFeatures, const TBassMetaConstScheme& meta);
bool HasBiometricsScores(const TBassMetaConstScheme& meta);
bool HasNonEmptyBiometricsScores(const TBassMetaConstScheme& meta);

} // namespace NAlice::NBiometry
