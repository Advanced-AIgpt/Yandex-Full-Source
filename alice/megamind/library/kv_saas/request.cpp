#include "request.h"

#include <alice/library/util/search_convert.h>
#include <alice/megamind/library/experiments/flags.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/string/join.h>

namespace NAlice::NKvSaaS {
namespace {

using TRequestKeys = TVector<TString>;

TStatus CreateRequest(const TRequestKeys& keys, NNetwork::IRequestBuilder& request) {
    request.AddCgiParam(TStringBuf("service"), TStringBuf("alice_personal"));
    request.AddCgiParam(TStringBuf("sgkps"), TStringBuf("0"));
    request.AddCgiParam(TStringBuf("ms"), TStringBuf("proto"));
    request.AddCgiParam(TStringBuf("sp_meta_search"), TStringBuf("multi_proxy"));
    request.AddCgiParam(TStringBuf("balancertimeout"), TStringBuf("90")); // See ALICEINFRA-562 + SAASSUP-3575 or contact i024@
    for (const auto& key : keys) {
        request.AddCgiParam(TStringBuf("text"), key);
    }
    return Success();
}

} // namespace

TSourcePrepareStatus CreatePersonalIntentsRequest(NMegamind::TRequestComponentsView<NMegamind::TClientComponent> skr,
                                                  NNetwork::IRequestBuilder& request)
{
    const bool isEnabled = skr.ClientFeatures().IsSmartSpeaker() && !skr.HasExpFlag(EXP_DISABLE_KV_SAAS);
    if (!isEnabled) {
        return ESourcePrepareType::NotNeeded;
    }

    if (auto e = CreateRequest({ConvertUuidForSearch(skr.ClientFeatures().Uuid)}, request)) {
        return std::move(*e);
    }

    return ESourcePrepareType::Succeeded;
}

TSourcePrepareStatus CreateQueryTokensStatsRequest(const TString& utterance, NMegamind::TRequestComponentsView<NMegamind::TClientComponent> skr,
                                                   NNetwork::IRequestBuilder& request)
{
    const TRequestKeys tokens = StringSplitter(utterance).Split(' ').SkipEmpty();
    const bool isEnabled = skr.ClientFeatures().IsSmartSpeaker()
                           && skr.HasExpFlag(EXP_ENABLE_QUERY_TOKEN_STATS)
                           && !tokens.empty();
    if (!isEnabled) {
        return ESourcePrepareType::NotNeeded;
    }

    TVector<TString> allNgramTokens;

    for (const uint ngramSize : TVector<uint>{1,2}) {
        TDeque<TStringBuf> tokensDeque;

        for (const auto& token : tokens) {
            tokensDeque.push_back(token);
            if (tokensDeque.size() > ngramSize) {
                tokensDeque.pop_front();
            }

            if (tokensDeque.size() == ngramSize) {
                allNgramTokens.push_back(JoinSeq(" ", tokensDeque));
            }
        }
    }

    if (auto e = CreateRequest(allNgramTokens, request)) {
        return std::move(*e);
    }

    return ESourcePrepareType::Succeeded;
}

} // namespace NAlice::NKvSaaS
