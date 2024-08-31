#pragma once

#include "abuse.h"
#include "error.h"

#include <alice/bass/forms/external_skill/scheme.sc.h>
#include <alice/bass/forms/context/fwd.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NBASS {
namespace NExternalSkill {

/** It is used for helper and validation callback class for scheme.Validate()
 */
class TSkillValidateHelper {
public:
    const bool IsInternal;
    struct TAbusePath {
        TAbusePath(bool isForTTS, TStringBuf path)
            : IsForTTS(isForTTS)
            , Path(path)
        {
        }

        bool IsForTTS;
        TString Path;
    };
    TVector<TAbusePath> AbusePaths;

public:
    /** Errors collector!
     * @param[in] ctx is a context
     * @param[in] isInternal needs to know if the validation is for internal skill or not (verbose errors)
     */
    TSkillValidateHelper(bool isInternal)
        : IsInternal(isInternal)
    {
    }

    bool CheckForImageId(TStringBuf imageId, TStringBuf subPath);
    bool CheckForUrl(TStringBuf path, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, typename TSchemeTraits::TStringType> url);
    bool CheckForNotNull(TStringBuf path, const NSc::TValue& value);
    bool CheckForUtf8Size(TStringBuf path, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, typename TSchemeTraits::TStringType> value, ssize_t from, ssize_t to);
    bool CheckForSize(TStringBuf path, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, typename TSchemeTraits::TStringType> value, ssize_t from, ssize_t to);
    bool CheckForSize(TStringBuf path, NDomSchemeRuntime::TConstAnyValue<TSchemeTraits> value, ssize_t from, ssize_t to) {
        return CheckForSize(path, value.AsString(), from, to);
    }
    void AddApiError(ui32 id, TStringBuf msg, TStringBuf path = TStringBuf());

    void operator ()(TStringBuf key, TStringBuf errmsg, NDomSchemeRuntime::TValidateInfo vi);

    void AddProblem(TStringBuf type, TStringBuf path, TStringBuf msg);

    void AddForAbuseChecking(TStringBuf path, TStringBuf value, bool isForTTS = false) {
        if (value) {
            AbusePaths.emplace_back(TAbusePath{isForTTS, TString(path)});
        }
    }

    TErrorBlock::TResult Error;

private:
    NSc::TValue& Data();
};

} // namespace NExternalSkill
} // namespace NBASS
