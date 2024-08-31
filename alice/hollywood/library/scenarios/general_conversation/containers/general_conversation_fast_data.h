#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/begemot/lib/fixlist_index/fixlist_index.h>

namespace NAlice::NHollywood::NGeneralConversation {

struct LineCombinationCoefficients {
    double Relev;
    double Informativeness;
    double Interest;
    double NotRude;
    double NotMale;
    double Respect;
};

class TGeneralConversationFastData : public IFastData {
public:
    TGeneralConversationFastData(const TGeneralConversationFastDataProto& proto);

    void FilterCandidatesByResponseBanlist(TVector<TAggregatedReplyCandidate>* candidates) const;

    bool FilterRequest(const TString& request, const TString banlist = "gc_mini_request_banlist") const;

    void FilterGifByBanlist(TVector<const TGif*>* gifs) const;

    void CopyIfNotInBanlistFactsCrosspromo(const TVector<TString>& facts, TVector<TString>* filteredFacts) const;

    const TMap<TString, TMap<int32_t, LineCombinationCoefficients>>& GetCombinationCoefficientsDict() const;

private:
    NBg::TFixlistIndex ResponseBanlist;
    NBg::TFixlistIndex RequestBanlist;
    NBg::TFixlistIndex GifBanlist;
    NBg::TFixlistIndex FactsCrosspromoBanlist;
    TMap<TString, TMap<int32_t, LineCombinationCoefficients>> LineCombinationCoefficientsDict;
};

} // namespace NAlice::NHollywood::NGeneralConversation
