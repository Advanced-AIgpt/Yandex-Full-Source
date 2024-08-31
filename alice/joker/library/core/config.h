#pragma once

#include <alice/joker/library/backend/backend.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <util/folder/path.h>
#include <util/generic/fwd.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>

#include <alice/joker/library/core/config.sc.h>

namespace NAlice::NJoker {

/** Config parser class.
 * It is possible to use a small lua blocks within config.
 * The lua preprocessor is run before json parsing. The lua environment has
 * all the os environemnt variables with prefix 'ENV_'.
 * (i.e. if there is os env variable LC_ALL it is possible to get its value by writing "${ ENV_LC_ALL }").
 * If requested variable is undefined than constructor throws yexception.
 * @code
 * { "some_key": "${ ENV_SUPER_DUPER_ENV_VARIABLE }", "other_key": "ok" }
 * @endcode
 */
class TConfig : private NSc::TValue, public NJokerConfig::TConfig<TSchemeTraits>, NNonCopyable::TNonCopyable {
public:
    using TScheme = NJokerConfig::TConfig<TSchemeTraits>;
    using TKeyValues = TVector<TString>;
    using TOnJsonPatch = std::function<void(TScheme)>;

public:
    TConfig(const TString& in, const TKeyValues& cmdVariables, TMaybe<TOnJsonPatch> jsonPatcher = Nothing());
    TConfig(TConfig&& config);

    const TFsPath& SessionsPath() const;

    const THashSet<TString>& SkipHeaders() const {
        return SkipHeaders_;
    }

    const THashSet<TString>& SkipCGIs() const {
        return SkipCGIs_;
    }

    bool SkipAllHeaders() const {
        return SkipAllHeaders_;
    }

    bool SkipAllCGIs() const {
        return SkipAllGCIs_;
    }

    bool SkipBody() const {
        return SkipBody_;
    }

    THolder<NAlice::NJoker::TBackend> CreateBackend() const;

    using NSc::TValue::ToJsonPretty;
    using NSc::TValue::ToJson;

private:
    const TFsPath SessionsPath_;
    const THashSet<TString> SkipHeaders_;
    const THashSet<TString> SkipCGIs_;
    const bool SkipAllHeaders_ = false;
    const bool SkipAllGCIs_ = false;
    const bool SkipBody_ = false;
};

} // namespace NAlice::NJoker
