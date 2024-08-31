#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NTrNavi {

constexpr TStringBuf BEG_YOUR_PARDON = "beg_your_pardon";
constexpr TStringBuf TEMPLATE_GENERAL_CONVERSATION = "general_conversation";
constexpr TStringBuf GENERAL_CONVERSATION_FRAME = "alice.vinsless.general_conversation";

void PrepareResponseWithPhrase(TResponseBodyBuilder& responseBodyBuilder, TRTLogger& logger,
                               const TScenarioRunRequestWrapper& requestWrapper, TStringBuf phraseName);

} // namespace NAlice::NHollywood::NTrNavi

