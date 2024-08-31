#include "general_conversation_fast_data.h"

#include <alice/hollywood/library/gif_card/proto/gif.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>

namespace NAlice::NHollywood::NGeneralConversation {

TGeneralConversationFastData::TGeneralConversationFastData(const TGeneralConversationFastDataProto& proto) {
    TStringInput responseBanlistStream(proto.GetResponseBanlist());
    ResponseBanlist.AddFixlist("gc_response_banlist", &responseBanlistStream);

    TStringInput requestBanlistStream(proto.GetRequestBanlist());
    RequestBanlist.AddFixlist("gc_mini_request_banlist", &requestBanlistStream);

    TStringInput requestTaleBanlistStream(proto.GetRequestTaleBanlist());
    RequestBanlist.AddFixlist("gc_tale_request_banlist", &requestTaleBanlistStream);

    TStringInput gifBanlistStream(proto.GetGifResponseUrlBanlist());
    GifBanlist.AddFixlist("gc_gif_response_url_banlist", &gifBanlistStream);

    TStringInput factsCrosspromoBanlistStream(proto.GetFactsCrosspromoResponseBanlist());
    FactsCrosspromoBanlist.AddFixlist("gc_facts_crosspromo_banlist", &factsCrosspromoBanlistStream);
    auto tmpLineCombinationCoefficientsDict = proto.GetLineCombinationCoefficientsDict();
    for (const auto& pairCoefficientsSamplePerDay : tmpLineCombinationCoefficientsDict) {
        TMap<int32_t, LineCombinationCoefficients> tmpCoefficientsSamplePerDay;
        for (const auto& pairLineCombinationCoefficients : pairCoefficientsSamplePerDay.second.GetDailySetOfLineCombinations()) {
            LineCombinationCoefficients tmp;
            tmp.Relev = pairLineCombinationCoefficients.second.GetRelev();
            tmp.Informativeness = pairLineCombinationCoefficients.second.GetInformativeness();
            tmp.Interest = pairLineCombinationCoefficients.second.GetInterest();
            tmp.NotRude = pairLineCombinationCoefficients.second.GetNotRude();
            tmp.NotMale = pairLineCombinationCoefficients.second.GetNotMale();
            tmp.Respect = pairLineCombinationCoefficients.second.GetRespect();
            tmpCoefficientsSamplePerDay[pairLineCombinationCoefficients.first] = tmp;
        }
        LineCombinationCoefficientsDict[pairCoefficientsSamplePerDay.first] = tmpCoefficientsSamplePerDay;
    }
}

const TMap<TString, TMap<int32_t, LineCombinationCoefficients>>& TGeneralConversationFastData::GetCombinationCoefficientsDict() const {
    return this->LineCombinationCoefficientsDict;
}

void TGeneralConversationFastData::FilterCandidatesByResponseBanlist(TVector<TAggregatedReplyCandidate>* candidates) const {
    EraseIf(*candidates, [&] (const auto& candidate) { return !ResponseBanlist.MatchAgainst({GetAggregatedReplyText(candidate), "", {}}, "gc_response_banlist").empty(); });
}

void TGeneralConversationFastData::FilterGifByBanlist(TVector<const TGif*>* gifs) const {
    EraseIf(*gifs, [&] (const auto* gif) { return !GifBanlist.MatchAgainst({gif->GetUrl(), "", {}}, "gc_gif_response_url_banlist").empty(); });
}

bool TGeneralConversationFastData::FilterRequest(const TString& request, const TString banlist) const {
    return !RequestBanlist.MatchAgainst({request, "", {}}, banlist).empty();
}

void TGeneralConversationFastData::CopyIfNotInBanlistFactsCrosspromo(const TVector<TString>& facts, TVector<TString>* filteredFacts) const {
    CopyIf(facts.begin(), facts.end(), std::back_inserter(*filteredFacts),
            [&] (const auto& fact) { return FactsCrosspromoBanlist.MatchAgainst({fact, "", {}}, "gc_facts_crosspromo_banlist").empty(); });
}

} // namespace NAlice::NHollywood::NGeneralConversation
