#pragma once

#include <library/cpp/json/json_value.h>

#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/fwd.h>

namespace NAlice::NHollywood {

struct TNlgData {
    NJson::TJsonValue Context;
    NJson::TJsonValue ReqInfo; /* DEPRECATED it's better to use Context only */
    NJson::TJsonValue Form; /* DEPRECATED it's better to use Context only */

    TRTLogger& Logger;
    bool ShouldLogNlg = false;

    TNlgData(TRTLogger& logger, const TScenarioBaseRequestWrapper& request);
    TNlgData(TRTLogger& logger);

    void AddAttention(const TStringBuf attention);
};

} // NAlice::NHollywood
