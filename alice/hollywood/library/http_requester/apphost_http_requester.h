#pragma once

#include "http_requester.h"

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NAlice::NHollywood {

// Thrown on IHttpRequester::Start() when exectuted on an http proxy request preparation apphost node.
// When IHttpRequester::Start() is excetuded on http proxy result processing apphost node makes no effect.
class TAppHostNodeExecutionBreakException : public yexception {
};

THolder<IHttpRequester> MakeApphostHttpRequester(NAppHost::IServiceContext& serviceCtx,
    const NScenarios::TRequestMeta& requestMeta, const NJson::TJsonValue& apphostParams, TRTLogger& logger,
    const TString& typePrefix = "", const TString& nodePrefix = ""
);

} // namespace NAlice::NHollywood
