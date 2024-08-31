#pragma once

#include <alice/bass/forms/context/context.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS::NCrmbot {

class TPersonalDataHelper {
public:
    TPersonalDataHelper(const NBASS::TContext& ctx);

    TMaybe<i64> GetRegionId() const;
    TStringBuf GetName() const;
    TStringBuf GetEmail() const;
    TStringBuf GetPhone() const;

    TStringBuf GetIP() const;
    TStringBuf GetYandexUid() const;
    TStringBuf GetPuid() const;
    TStringBuf GetMuid() const;

private:
    const NSc::TValue& PersonalData;
};

}
