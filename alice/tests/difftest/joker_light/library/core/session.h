#pragma once

#include "status.h"
#include "fwd.h"

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NJokerLight {

class TSession {
public:
    struct TSettings {
        bool FetchIfNotExists;
        bool ImitateDelay;
        bool DontSave;

        TSettings()
            : FetchIfNotExists{false}
            , ImitateDelay{false}
            , DontSave{false}
        {
        }
    };

public:
    TSession(TContext& context, const TString& id);

    void Init(TSettings settings);
    TStatus Load();

    const TSettings& Settings() const;
    const TString& Id() const;

private:
    TContext& Context_;
    TString Id_;
    TMaybe<TSettings> Settings_;
};

} // namespace NAlice::NJokerLight
