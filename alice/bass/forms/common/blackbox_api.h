#pragma once

#include "personal_data.h"

#include <alice/bass/forms/context/context.h>

#include <util/generic/string.h>

namespace NBASS {

// A lightweight polymorphic wrapper for TPersonalDataHelper, mostly
// needed to mock communications with BlackBox in tests.
class TBlackBoxAPI {
public:
    virtual ~TBlackBoxAPI() = default;

    virtual bool GetUid(TContext& context, TString& uid);
    virtual bool GetUserInfo(TContext& context, TPersonalDataHelper::TUserInfo& userInfo);
};

// Mocked version of TBlackBoxApi
struct TBlackBoxAPIFake : public TBlackBoxAPI {
    TBlackBoxAPIFake() = default;

    explicit TBlackBoxAPIFake(TStringBuf uid);

    bool GetUid(TContext& /* ctx */, TString& uid);
    bool GetUserInfo(TContext& /* ctx */, TPersonalDataHelper::TUserInfo& userInfo);

    TMaybe<TString> UID;
    size_t NumCalls = 0;
};

} // namespace NBASS
