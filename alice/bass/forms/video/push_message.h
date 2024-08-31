#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/video/defs.h>

namespace NBASS::NVideo {

struct PushMessageParams {
    TString Id;
    TString Tag;
    TString Text;
    TString ThrottlePolicy;
    TString Title;
    TString Url;
    THashMap<TString, TString> AdditionalParams;
};


void AddPushMessageResponseDirective(TContext& ctx, const PushMessageParams& pushMessageParams);
void AddBuyPushMessageResponseDirective(TContext& ctx, TVideoItemConstScheme item, const TCgiParameters& addParams = TCgiParameters());

void AddShowBuyPushScreenResponseDirective(TContext& ctx, TVideoItemConstScheme item, const TCgiParameters& addParams = TCgiParameters());

void AddBuyPushScreenAndPushMessageDirectives(TContext& ctx, TVideoItemConstScheme item, const TCgiParameters& addParams = TCgiParameters());

}
